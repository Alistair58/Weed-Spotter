#ifndef CAMERAACCESS_HPP
#define CAMERAACCESS_HPP

#include <libcamera/libcamera.h>
#include <libcamera/controls.h>
#include <libcamera/camera_manager.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <libcamera/framebuffer.h>
#include <libcamera/base/signal.h>
#include <future>
#include "cameraimage.hpp"


using namespace libcamera;

class CameraAccess{
	public:
		CameraAccess();
		~CameraAccess();
		CameraImage takePhoto();
		void photoRequestComplete(Request *req);
	private:
		CameraManager cm;
		std::shared_ptr<Camera> camera;
		std::unique_ptr<FrameBufferAllocator> allocator;
		std::promise<void> photoPromise;
		Stream *stream;
		ControlList controls;
		int imageHeight = 480;
		int imageWidth = 640;
};

#endif
