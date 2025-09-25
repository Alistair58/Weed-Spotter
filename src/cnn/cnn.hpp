#ifndef CNN_HPP
#define CNN_HPP

#include "globals.hpp"
#include <vector>
#include "tensor.hpp"
#include "cnnutils.hpp"
#if PROFILING
	#include "timer.hpp"
#endif

class CNN : public CnnUtils{
    public:
        //CONSTRUCTORS 
        //Creating a fresh CNN
        CNN(d2& pixelStats);
        //Creating a copy from an original CNN
        CNN(CNN *original,bool deepCopyWeights);
    
        //KEY METHODS 
        //returns {weedX,weedY,hasWeedProbability}
        std::vector<float> forwards(Tensor& imageInt
        #if PROFILING
            ,Timer *parentTimer = nullptr
        #endif 
        );
};

#endif
