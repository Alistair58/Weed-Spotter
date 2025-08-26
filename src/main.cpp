#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/camera.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer.h>
#include <libcamera/base/signal.h>
#include "cameraimage.hpp"
#include "cameraaccess.hpp"

int main(int argc,char **argv){
	CameraAccess cameraAccess;
	CameraImage image = cameraAccess.takePhoto();
	image.saveImage("img.jpg");
	return 0;
}
