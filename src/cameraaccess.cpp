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
	CameraConfiguration config = camera->generateConfiguration({StreamRole::StillCapture});
	if(config.size()==0){
		throw std::runtime_error("Failed to generate camera configuration");
	}
	config[0].pixelFormat = PixelFormat::Jpeg;
	//8MP up to 3280x2646, ratio = 1.33
	config[0].size.width = 640;
	config[0].size.height = 480;
	if(camera->configure(&config) != 0){
		throw std::runtime_error("Failed to configure camera");
	}
	camera->start();
	
	allocator = std::make_unqiue<FrameBufferAllocator>(camera);
	Stream *stream = config[0].stream();
	if(allocator->allocate(stream) != 0){
		throw std::runtime_error("Failed to allocate camera buffer");
	}
	buffers = allocator->buffers(stream);
}

CameraAccess::~CameraAccess(){
	allocator.reset();
	camera->stop();
	camera->release();
	cm.stop();
}

CameraImage CameraAccess::takePhoto(){
	Request *request = camera->createRequest();
	if(!request){
		throw std::runtime_error("Failed to create camera request");
	}
	request->addBuffer(stream,buffers[0].get());
	//Wait for photo request to complete
	photoPromise = std::promise<void>();
	std::future<void> future  = photoPromise.get_future();
	camera->requestCompleted.connect(this,&CameraAccess::photoRequestComplete);
	camera->queueRequest(request);
	future.wait();
	
	//FrameBuffer::Plane is a chunk of memory where image data is written to
	//JPEG is stored in a single plane
	const FrameBuffer::Plane plane = buffers[0]->planes()[0];
	void *planeAddr = mmap(nullptr,plane.length,PROT_READ,MAP_SHARED,plane.fd.get(),plane.offset);
	if(planeAddr == MAP_FAILED) throw std::runtime_error("Plane mmap failed");
	//The photo may not use all the bytes in the plane
	size_t bytesUsed = buffers[0]->metadata().planes()[0].bytesused;
	delete request;
	return CameraImage(reinterpret_cast<char*>(planeAddr),bytesUsed);
}

void CameraAccess::photoRequestComplete(Request *req){
	if(req->status() == Request::RequestComplete){
		photoPromise.set_value(); //Done
	}
}


