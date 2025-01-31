/*
 * openvrdevice.h
 *
 *  Created on: Dec 18, 2015
 *      Author: Chris Denham
 */

#ifndef _OSG_OPENVRDEVICE_H_
#define _OSG_OPENVRDEVICE_H_

// Include the OpenVR SDK
#include <openvr.h>

#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/Version>
#include <osg/FrameBufferObject>

#if(OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0))
	typedef osg::GLExtensions OSG_GLExtensions;
	typedef osg::GLExtensions OSG_Texture_Extensions;
#else
	typedef osg::FBOExtensions OSG_GLExtensions;
	typedef osg::Texture::Extensions OSG_Texture_Extensions;
#endif

class OpenVRTextureBuffer : public osg::Referenced
{
	public:
		OpenVRTextureBuffer(osg::ref_ptr<osg::State> state, int width, int height, int msaaSamples);
		void destroy(osg::GraphicsContext* gc);
		GLuint getTexture() { return m_Resolve_ColorTex; }
		GLuint getMSAAColor() { return m_MSAA_ColorTex; }
		GLuint getMSAADepth() { return m_MSAA_DepthTex; }
		int textureWidth() const { return m_width; }
		int textureHeight() const { return m_height; }
		int samples() const { return m_samples; }
		void onPreRender(osg::RenderInfo& renderInfo);
		void onPostRender(osg::RenderInfo& renderInfo);

	protected:
		~OpenVRTextureBuffer() {}

		friend class OpenVRMirrorTexture;
		GLuint m_Resolve_FBO; // MSAA FBO is copied to this FBO after render.
		GLuint m_Resolve_ColorTex; // color texture for above FBO.
		GLuint m_MSAA_FBO; // framebuffer for MSAA RTT
		GLuint m_MSAA_ColorTex; // color texture for MSAA RTT 
		GLuint m_MSAA_DepthTex; // depth texture for MSAA RTT
		GLint m_width; // width of texture in pixels
		GLint m_height; // height of texture in pixels
		int m_samples;  // sample width for MSAA

};

class OpenVRMirrorTexture : public osg::Referenced
{
	public:
		enum BlitOptions
		{
			BOTH_EYES,
			LEFT_EYE,
			RIGHT_EYE
		};

		GLint width() { return m_width; }
		GLint height() { return m_height; }

		OpenVRMirrorTexture(osg::ref_ptr<osg::State> state, GLint width, GLint height);
		void destroy(osg::GraphicsContext* gc);
		void blitTexture(osg::GraphicsContext* gc, OpenVRTextureBuffer* leftEye,  OpenVRTextureBuffer* rightEye, BlitOptions eye = BlitOptions::BOTH_EYES);
	protected:
		~OpenVRMirrorTexture() {}

		GLuint m_mirrorFBO;
		GLuint m_mirrorTex;
		GLint m_width;
		GLint m_height;
};


class OpenVRPreDrawCallback : public osg::Camera::DrawCallback
{
	public:
		OpenVRPreDrawCallback(osg::Camera* camera, OpenVRTextureBuffer* textureBuffer)
			: m_camera(camera)
			, m_textureBuffer(textureBuffer)
		{
		}

		virtual void operator()(osg::RenderInfo& renderInfo) const;
	protected:
		osg::Camera* m_camera;
		OpenVRTextureBuffer* m_textureBuffer;

};

class OpenVRPostDrawCallback : public osg::Camera::DrawCallback
{
	public:
		OpenVRPostDrawCallback(osg::Camera* camera, OpenVRTextureBuffer* textureBuffer)
			: m_camera(camera)
			, m_textureBuffer(textureBuffer)
		{
		}

		virtual void operator()(osg::RenderInfo& renderInfo) const;
	protected:
		osg::Camera* m_camera;
		OpenVRTextureBuffer* m_textureBuffer;

};

class OpenVRDevice : public osg::Referenced
{

	public:
		typedef enum _Button
		{
			MENU = 1,
			GRIP = 2,
			PAD = 32,
			TRIGGER = 33
		} Button;

		typedef enum Eye_
		{
			LEFT = 0,
			RIGHT = 1,
			COUNT = 2
		} Eye;

		typedef struct _ControllerData
		{
			// Fields to be initialzed by iterateAssignIds() and setHands()
			int deviceID = -1;  // Device ID according to the SteamVR system
			int hand = -1;      // 0=invalid 1=left 2=right
			int idtrigger = -1; // Trigger axis id
			int idpad = -1;     // Touchpad axis id

			// Analog button data to be set in ContollerCoods()
			float padX;
			float padY;
			float trigVal;

			bool menuPressed = false;
			bool gripPressed = false;
			bool padPressed = false;
			bool triggerPressed = false;

			osg::Vec3 position;
			osg::Quat rotation;

			bool isValid;
		} ControllerData;

		typedef struct _TrackerData 
		{
			int deviceID = -1; // Device ID according to the SteamVR system
			osg::Vec3 position;
			osg::Quat rotation;
			bool isValid;
		} TrackerData;

		ControllerData controllers[2];
		TrackerData trackers[32];
		int hmdDeviceID = -1;
		int numControllers = 2;
		int numTrackers = 0;


		OpenVRDevice(float nearClip, float farClip, const float worldUnitsPerMetre = 1.0f, const int samples = 0);
		void createRenderBuffers(osg::ref_ptr<osg::State> state);
		void init();
		void calculateEyeAdjustment();
		void calculateProjectionMatrices();
		void shutdown(osg::GraphicsContext* gc);

		static bool hmdPresent();
		bool hmdInitialized() const;

		osg::Matrix projectionMatrixCenter() const;
		osg::Matrix projectionMatrixLeft() const;
		osg::Matrix projectionMatrixRight() const;

		osg::Matrix projectionOffsetMatrixLeft() const;
		osg::Matrix projectionOffsetMatrixRight() const;

		osg::Matrix viewMatrixLeft() const;
		osg::Matrix viewMatrixRight() const;

		float nearClip() const { return m_nearClip;	}
		float farClip() const { return m_farClip; }
		void setNearClip(const float nearclip) { m_nearClip = nearclip; }
		void setFarClip(const float farclip) { m_farClip = farclip; }

		void resetSensorOrientation() const;
		void updatePose();

		void assignIDs();
		void updateControllerEvents();

		osg::Vec3 position() const { return m_position; }
		osg::Quat orientation() const { return m_orientation;  }

		osg::Camera* createRTTCamera(OpenVRDevice::Eye eye, osg::Transform::ReferenceFrame referenceFrame, const osg::Vec4& clearColor, osg::GraphicsContext* gc = 0) const;

		bool submitFrame();
		void blitMirrorTexture(osg::GraphicsContext* gc, OpenVRMirrorTexture::BlitOptions eye = OpenVRMirrorTexture::BlitOptions::BOTH_EYES);

		vr::IVRSystem* vrSystem() const { return m_vrSystem; }

		osg::ref_ptr<OpenVRTextureBuffer> m_textureBuffer[2];
		osg::ref_ptr<OpenVRMirrorTexture> m_mirrorTexture;

		osg::GraphicsContext::Traits* graphicsContextTraits() const;
	protected:
		~OpenVRDevice(); // Since we inherit from osg::Referenced we must make destructor protected

		void trySetProcessAsHighPriority() const;

		osg::Matrixf m_leftEyeProjectionMatrix;
		osg::Matrixf m_rightEyeProjectionMatrix;
		osg::Vec3f m_leftEyeAdjust;
		osg::Vec3f m_rightEyeAdjust;

		osg::Vec3 m_position;
		osg::Quat m_orientation;

		float m_nearClip;
		float m_farClip;
		int m_samples;

		vr::IVRSystem* m_vrSystem;
		vr::IVRRenderModels* m_vrRenderModels;
		const float m_worldUnitsPerMetre;
	private:
		std::string GetDeviceProperty(vr::TrackedDeviceProperty prop);
		OpenVRDevice(const OpenVRDevice&); // Do not allow copy
		OpenVRDevice& operator=(const OpenVRDevice&); // Do not allow assignment operator.
};


class OpenVRRealizeOperation : public osg::GraphicsOperation
{
	public:
		explicit OpenVRRealizeOperation(osg::ref_ptr<OpenVRDevice> device) :
			osg::GraphicsOperation("OpenVRRealizeOperation", false), m_device(device), m_realized(false) {}
		virtual void operator () (osg::GraphicsContext* gc);
		bool realized() const { return m_realized; }
	protected:
		OpenThreads::Mutex  _mutex;
		osg::observer_ptr<OpenVRDevice> m_device;
		bool m_realized;
};


class OpenVRSwapCallback : public osg::GraphicsContext::SwapCallback
{
	public:
		explicit OpenVRSwapCallback(osg::ref_ptr<OpenVRDevice> device) : m_device(device), m_frameIndex(0) {}
		void swapBuffersImplementation(osg::GraphicsContext* gc);
		int frameIndex() const { return m_frameIndex; }
	private:
		osg::ref_ptr<OpenVRDevice> m_device;
		int m_frameIndex;
};


#endif /* _OSG_OPENVRDEVICE_H_ */
