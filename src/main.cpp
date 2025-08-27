#include "cameraimage.hpp"
#include "cameraaccess.hpp"

int main(int argc,char **argv){
	CameraAccess cameraAccess;
	CameraImage image = cameraAccess.takePhoto();
	image.saveAsJPEG("img.jpg");
	return 0;
}
