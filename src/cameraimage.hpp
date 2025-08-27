#ifndef CAMERAIMAGE_HPP
#define CAMERAIMAGE_HPP

#include <iostream>
#include <fstream>
#include <memory>
#include <exception>
#include <stdio.h>
#include <jpeglib.h>

class CameraImage{
	public:
		std::unique_ptr<uint8_t[]> data = nullptr; //HxWxC
		int height = -1;
		int width = -1;
		CameraImage(std::unique_ptr<uint8_t[]>& inputData,int inputHeight,int inputWidth){
			if(inputHeight<=0 || inputWidth<=0){
				throw std::invalid_argument("CameraImage dimensions must be positive");
			}
			this->data = std::move(inputData);
			this->height = inputHeight;
			this->width = inputWidth;
		}
		void saveAsJPEG(std::string fname);
		static CameraImage YUYVToRGB(uint8_t *data,int height,int width);
};

#endif
