#ifndef CNNUTILS_HPP
#define CNNUTILS_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <numbers>
#include "globals.hpp"
#include "tensor.hpp"
#include <arm_neon.h>

#if PROFILING
    #include "timer.hpp"
#endif

typedef struct dimens{
    int c;
    int h;
    int w;
}dimens;

class CNN; //forward declaration needed for compilation of applyGradients

class CnnUtils {
    protected:
        //Things with mutliple layers are stored as vectors as each layer can have different sized tensors
        std::vector<Tensor> kernels; //the kernels are stored [layer][currLayerChannel][prevLayerChannel][y][x] 
        std::vector<Tensor> activations;
        std::vector<Tensor> weights;
        std::vector<Tensor> maps; //Note: the input image is included in "maps" for simplicity
        std::vector<Tensor> paddedMaps; //Reusing padding is better than allocating for every convolutions
        d2 pixelStats;
        std::vector<int> numNeurons;
        std::vector<dimens> mapDimens; //c,h,w - includes the result of pooling (except final pooling)
        std::vector<std::pair<int,int>> kernelSizes; //0 represents a pooling layer, the last one is excluded
        std::vector<std::pair<int,int>> strides; //pooling strides are included
        std::vector<std::unique_ptr<int[]>> maxPoolIndices;
        bool padding;

        //UTILS
        void reset();
        std::vector<Tensor> loadKernels(
        #if PROFILING
            Timer *parentTimer = nullptr
        #endif 
        );
        std::vector<Tensor> loadWeights(
        #if PROFILING
            Timer *parentTimer = nullptr
        #endif 
        );

    public:
        //IMAGE-RELATED
        Tensor parseImg(const Tensor& img
        #if PROFILING
            ,Timer *parentTimer = nullptr
        #endif
        ) const;
        void normaliseImg(Tensor& img
        #if PROFILING
            ,Timer *parentTimer = nullptr
        #endif 
        );
        static Tensor gaussianBlurKernel(int width,int height);
        static Tensor maxPool(Tensor& image,int xStride,int yStride);
        Tensor maxPool(Tensor& image,int xStride,int yStride,int *maxPoolIndices);
        //variable size output
        static Tensor convolution(const Tensor& image,Tensor& kernel,int xStride,int yStride,bool padding
        #if PROFILING
            ,Timer *parentTimer = nullptr
        #endif
        );
        //Saving the padding allocation
        //prePaddingImage doesn't contain the image data, it just needs to be the correct size
        Tensor convolution(const Tensor& image,Tensor& prePaddedImage,Tensor& kernel,const int xStride,const int yStride
        #if PROFILING
            ,Timer *parentTimer
        #endif
        );
        //fixed size output
        static Tensor convolution(Tensor& image,Tensor& kernel,int xStride,int yStride,int newWidth,int newHeight,bool padding
        #if PROFILING
            ,Timer *parentTimer = nullptr
        #endif
        );

        //MATH UTILS
        static std::vector<float> softmax(std::vector<float> inp);
        static inline float sigmoid(float num){
            if (num > 200) return 1;
            if (num < -200) return 0;
            return 1 / (float) (1 + std::exp(-num));
        }
        static inline float relu(float num){
            if (num <= 0) return 0;
            return num;
        }
        static inline float leakyRelu(float num){
            if (num <= 0) return num*0.01f;
            return num;
        }
        static inline bool floatCmp(float x,float y,float epsilon = std::numeric_limits<float>::min()){
            return (x+epsilon>=y && x-epsilon<=y);
        }
       
        static inline float dotProduct4f(float *X,float *Y);
        static inline float dotProduct4f(float32x4_t a,float32x4_t b);
        static inline float horizontalSum(float32x4_t a);

        //(GET|SET)TERS
        std::vector<dimens> getMapDimens() const{ return mapDimens; }
};

inline float CnnUtils::dotProduct4f(float *X,float *Y){
    float32x4_t a = vld1q_f32(X);       // Load 4 floats
    float32x4_t b = vld1q_f32(Y);       // Load 4 floats
    return dotProduct4f(a,b);
}

inline float CnnUtils::dotProduct4f(float32x4_t a,float32x4_t b){
    float32x4_t prod = vmulq_f32(a, b);   // Multiply X[i] * Y[i]
    //Now horizontally sum all 8 floats in prod
    return horizontalSum(prod);
}

inline float CnnUtils::horizontalSum(float32x4_t a){
    return vaddvq_f32(a);
}


#endif
