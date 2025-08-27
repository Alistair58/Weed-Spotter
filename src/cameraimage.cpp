#include "cameraimage.hpp"


CameraImage CameraImage::YUYVToRGB(uint8_t *data,int height,int width){
	int dataSize = width*height*2;
	int outputSize = width*height*3;
	std::unique_ptr<uint8_t[]> output = std::unique_ptr<uint8_t[]>(new uint8_t[outputSize]);
	for(int i=0,j=0;i<dataSize;i+=4){
		int Y0 = data[i]; //Pixel 0 brightness
		int U = data[i+1]; //Shared chrominanace blue-difference component
		int Y1 = data[i+2]; //Pixel 1 brightness
		int V = data[i+3]; //Shared chrominanace red-difference component
		//Used for calculations - no real world significance
		int C = Y0;
		int D = U-128;
		int E = V-128;

		auto clip = [](int val){
			return std::max(0,std::min(255,val));
		};
		//Pixel 0
		output[j++] = clip((298*C+409*E+128)>>8);
		output[j++] = clip((298*C-100*D-208*E+128)>>8);
		output[j++] = clip((298*C+516*D+128)>>8);
		//Pixel 1
		C = Y1;
		output[j++] = clip((298*C+409*E+128)>>8);
		output[j++] = clip((298*C-100*D-208*E+128)>>8);
		output[j++] = clip((298*C+516*D+128)>>8);
	}
	CameraImage result(output,height,width);
	return result;
}

void CameraImage::saveAsJPEG(std::string fname){
	FILE *f = fopen(fname.c_str(),"wb");
	if(!f){
		throw std::runtime_error("Could not open file "+fname);
	}
	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo,f);

	cinfo.image_width = this->width;
	cinfo.image_height = this->height;
	cinfo.input_components = 3; //RGB
	cinfo.in_color_space = JCS_RGB;

	const int quality = 75; //0 to 100
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo,quality,true); //the true is to force entries between 0 and 255
	jpeg_start_compress(&cinfo,true); // the true is to write the Huffman tables

	JSAMPROW rowPtr[1];
	int rowStride = this->width*3;
	while(cinfo.next_scanline < cinfo.image_height){
		rowPtr[0] = &this->data[cinfo.next_scanline * rowStride];
		jpeg_write_scanlines(&cinfo,rowPtr,1);
	}
	std::cout << "Saved " << fname << std::endl;
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(f);
}
