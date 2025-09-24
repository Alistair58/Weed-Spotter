#ifndef CNN_HPP
#define CNN_HPP

#include <string>
#include <unordered_map>
#include "tensor.hpp"
#include "cnnutils.hpp"
#include <arm_neon.h>

class CNN : public CnnUtils{
    public:
        //CONSTRUCTORS 
        //Creating a fresh CNN
        CNN(d2& pixelStats);
        //Creating a copy from an original CNN
        CNN(CNN *original);
    
        //KEY METHODS 
        //returns {weedX,weedY,hasWeedProbability}
        std::vector<float> forwards(Tensor& imageInt,bool training
        #if PROFILING
            ,Timer *parentTimer = nullptr
        #endif 
        );
};

#endif