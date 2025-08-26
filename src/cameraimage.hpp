#ifndef CAMERAIMAGE_HPP
#define CAMERAIMAGE_HPP

#include <iostream>
#include <fstream>

class CameraImage{
	public:
		char *data;
		size_t bytes;
		CameraImage(char *dataAddr,size_t numBytes){
			this->data = dataAddr;
			this->bytes = numBytes;
		}
		void saveImage(std::string fname){
			std::ofstream outfile(fname,std::ios::binary);
			outfile.write(reinterpret_cast<const char*>(this->data),this->bytes);
			outfile.close();
			std::cout << "Saved " << fname << std::endl;
		}
};

#endif
