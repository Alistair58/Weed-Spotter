#include "cameraaccess.hpp"

using namespace libcamera;

CameraAccess::CameraAccess(){
	//init everything we need
	cm.start();
	if(cm.cameras().empty()){
		throw std::runtime_error("Camera is not connected");
	}
	camera = cm.get(cm.cameras()[0]->id());
	//Acquire means that only this app can use this camera
	camera->acquire();
	std::cout << "Camera acquired: " << camera->id() << std::endl;

	//Tell it that we're taking a photo
	std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration({StreamRole::StillCapture});
	if(config->size()==0){
		throw std::runtime_error("Failed to generate camera configuration");
	}
	//8MP up to 3280x2646, ratio = 1.33
	//If you set it to MJPEG, it'll convert it to YUYV anyway
	config->at(0).pixelFormat = formats::YUYV;
	config->at(0).size.width = this->imageWidth;
	config->at(0).size.height = this->imageHeight;
	//Validate can adjust the camera configuration

	CameraConfiguration::Status status = config->validate();
	if(status == CameraConfiguration::Invalid){
		throw std::runtime_error("Camera configuration is invalid");
	}
	else if(status = CameraConfiguration::Adjusted){
		std::cout << "Camera configuration adjusted" << std::endl;
		this->imageWidth = config->at(0).size.width;
		this->imageHeight = config->at(0).size.height;
		if(config->at(0).pixelFormat != formats::YUYV){
			throw std::runtime_error("Libcamera changed format to "+config->at(0).pixelFormat.toString()+". YUYV is the only accepted format");
		}
	}

	if(camera->configure(config.get()) < 0){
		throw std::runtime_error("Failed to configure camera");
	}

	controls.set(controls::AeEnable,true);
	controls.set(controls::AwbEnable,true);

	allocator = std::make_unique<FrameBufferAllocator>(camera);
	stream = config->at(0).stream();
}

CameraAccess::~CameraAccess(){
	//Can be called if the camera is not running
	if(camera){
		camera->stop();
		camera->release();
	}
	allocator.reset();
	//The CameraManager destructor sorts itself out
}

CameraImage CameraAccess::takePhoto(){
	if(allocator->allocate(stream) < 0){
		throw std::runtime_error("Failed to allocate camera buffer");
	}
	const std::vector<std::unique_ptr<FrameBuffer>>& buffers = allocator->buffers(stream);
	std::unique_ptr<Request> request = camera->createRequest();
	if(request.get()==nullptr){
		throw std::runtime_error("Failed to create camera request");
	}
	request->addBuffer(stream,buffers[0].get());
	request->controls() = controls;
	//Wait for photo request to complete
	photoPromise = std::promise<void>();
	std::future<void> future  = photoPromise.get_future();
	camera->requestCompleted.connect(this,&CameraAccess::photoRequestComplete);
	//We will discard the first numDiscard photos to allow the auto exposure to stabilise
	//This is actually necessary (at least for the first photo)
	camera->start();
	camera->queueRequest(request.get());
	future.wait();
	const int numDiscard = 5;
	for(int i=0;i<numDiscard;i++){
		request->reuse();
		request->addBuffer(stream,buffers[0].get());
		request->controls() = controls;
		photoPromise = std::promise<void>();
		future = photoPromise.get_future();
		camera->queueRequest(request.get());
		future.wait();
	}
	//FrameBuffer::Plane is a chunk of memory where image data is written to
	//JPEG is stored in a single plane
	const FrameBuffer::Plane plane = buffers[0]->planes()[0];
	void *planeAddr = mmap(nullptr,plane.length,PROT_READ,MAP_SHARED,plane.fd.get(),plane.offset);
	if(planeAddr == MAP_FAILED) throw std::runtime_error("Plane mmap failed");
	//The photo may not use all the bytes in the plane
	size_t bytesUsed = buffers[0]->metadata().planes()[0].bytesused;
	if((int)bytesUsed != this->imageHeight*this->imageWidth*2){
		throw std::runtime_error("Image data does not match dimensions");
	}
	CameraImage result = CameraImage::YUYVToRGB(reinterpret_cast<uint8_t*>(planeAddr),this->imageHeight,this->imageWidth);
	camera->stop();
	return result;
}

void CameraAccess::photoRequestComplete(Request *req){
	if(req->status() == Request::RequestComplete){
		photoPromise.set_value(); //Done
	}
}


