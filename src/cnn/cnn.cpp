#include "cnn.hpp"
#include <arm_neon.h>

//----------------------------------------------------
//CONSTRUCTORS 

// Model 6:
// 3x480x640 -32x6x8-> 
// 32x80x80 -32x3x3->
// 32x40x40 -64x3x3->
// 64x20x20 -128x3x3->
// 128x10x10 -2x2 MP->
// 128x5x5 -> 3200 -FC->
// 512 -FC-> 3

//Creating a fresh CNN
CNN::CNN(d2& pixelStatsInp){
    numNeurons = {3200,512,3};
    //includes the result of pooling (except final pooling)
    mapDimens = std::vector<dimens>(5);
    mapDimens[0] = {3,480,640};
    mapDimens[1] = {32,80,80};
    mapDimens[2] = {32,40,40};
    mapDimens[3] = {64,20,20};
    mapDimens[4] = {128,10,10};
    //0 represents a pooling layer, the last one is excluded
    kernelSizes = std::vector<std::pair<int,int>>(4);
    kernelSizes[0] = {6,8}; //h,w
    kernelSizes[1] = {3,3};
    kernelSizes[2] = {3,3};
    kernelSizes[3] = {3,3};
    //pooling strides are included
    strides = std::vector<std::pair<int,int>>(5);
    strides[0] = {6,8};//y,x - pooling strides are included
    strides[1] = {2,2};
    strides[2] = {2,2};
    strides[3] = {2,2};
    strides[4] = {2,2};
    padding = true;

    this->pixelStats = pixelStatsInp;
    this->kernels = loadKernels();
    this->weights = loadWeights();
    this->activations = std::vector<Tensor>(numNeurons.size());
    for(int l=0;l<numNeurons.size();l++){
        activations[l] = Tensor({numNeurons[l]});
    }
    this->maps = std::vector<Tensor>(mapDimens.size());
    for(int l=0;l<mapDimens.size();l++){
        maps[l] = Tensor({mapDimens[l].c,mapDimens[l].h,mapDimens[l].w});
    }
    if(padding){
        this->paddedMaps = std::vector<Tensor>(mapDimens.size()-1); //last map is pooled not convolved - my favourite way of having a Martini
        for(int l=0;l<mapDimens.size()-1;l++){
            int kernelRadiusY = std::floor(this->kernelSizes[l].first/2);
            int kernelRadiusX = std::floor(this->kernelSizes[l].second/2);
            int paddedHeight = mapDimens[l].h+2*kernelRadiusY;
            int paddedWidth = mapDimens[l].w+2*kernelRadiusX;
            paddedMaps[l] = Tensor({mapDimens[l].c,paddedHeight,paddedWidth});
        }
    }
    for(int l=0;l<kernelSizes.size();l++){
        if(kernelSizes[l].first==0 || kernelSizes[l].second==0){ //pooling
            int pooledDimenX = mapDimens[l].w/strides[l].second;
            int pooledDimenY = mapDimens[l].h/strides[l].first;
            maxPoolIndices.push_back(std::unique_ptr<int[]>(new int[mapDimens[l].c*pooledDimenY*pooledDimenX]));
        }
    }
    //final pooling
    int finalPooledDimenX = mapDimens[mapDimens.size()-1].w/strides[strides.size()-1].second;
    int finalPooledDimenY = mapDimens[mapDimens.size()-1].h/strides[strides.size()-1].first;
    maxPoolIndices.push_back(std::unique_ptr<int[]>(
        new int[mapDimens[mapDimens.size()-1].c*finalPooledDimenY*finalPooledDimenX]
    ));
}

//Creating a copy from a template CNN (I can't call it template)
CNN::CNN(CNN *original,bool deepCopyWeights) {
    numNeurons = original->numNeurons;
    mapDimens = original->mapDimens;
    kernelSizes = original->kernelSizes;
    strides = original->strides;
    padding = original->padding;
    pixelStats = original->pixelStats;
    if(deepCopyWeights){
        kernels = original->kernels; //copy by value
        weights = original->weights;
    }
    else{ //i.e. shallow copy
        this->kernels = std::vector<Tensor>(original->kernels.size());
        for(int i=0;i<original->kernels.size();i++){
            this->kernels[i].shallowCopy(original->kernels[i]);
        }
        this->weights = std::vector<Tensor>(original->weights.size());
        for(int i=0;i<original->weights.size();i++){
            this->weights[i].shallowCopy(original->weights[i]);
        }
    }
    this->activations = std::vector<Tensor>(numNeurons.size());
    for(int l=0;l<numNeurons.size();l++){
        activations[l] = Tensor({numNeurons[l]});
    }
    this->maps = std::vector<Tensor>(mapDimens.size());
    for(int l=0;l<mapDimens.size();l++){
        maps[l] = Tensor({mapDimens[l].c,mapDimens[l].h,mapDimens[l].w});
    }
    if(padding){
        this->paddedMaps = std::vector<Tensor>(mapDimens.size()-1); //last map is pooled not convolved - my favourite way of having a Martini
        for(int l=0;l<mapDimens.size()-1;l++){
            int kernelRadiusY = std::floor(this->kernelSizes[l].first/2);
            int kernelRadiusX = std::floor(this->kernelSizes[l].second/2);
            int paddedHeight = mapDimens[l].h+2*kernelRadiusY;
            int paddedWidth = mapDimens[l].w+2*kernelRadiusX;
            paddedMaps[l] = Tensor({mapDimens[l].c,paddedHeight,paddedWidth});
        }
    }
    for(int l=0;l<kernelSizes.size();l++){
        if(kernelSizes[l].first==0 || kernelSizes[l].second==0){ //pooling
            int pooledDimenX = mapDimens[l].w/strides[l].second;
            int pooledDimenY = mapDimens[l].h/strides[l].first;
            maxPoolIndices.push_back(std::unique_ptr<int[]>(new int[mapDimens[l].c*pooledDimenY*pooledDimenX]));
        }
    }
    //final pooling
    int finalPooledDimenX = mapDimens[mapDimens.size()-1].w/strides[strides.size()-1].second;
    int finalPooledDimenY = mapDimens[mapDimens.size()-1].h/strides[strides.size()-1].first;
    maxPoolIndices.push_back(std::unique_ptr<int[]>(
        new int[mapDimens[mapDimens.size()-1].c*finalPooledDimenY*finalPooledDimenX]
    ));
}


//----------------------------------------------------
//KEY METHODS 


std::vector<float> CNN::forwards(Tensor& imageInt
#if PROFILING
    ,Timer *parentTimer
#endif
){
    #if PROFILING
        Timer *forwardsTimer = parentTimer->addChildTimer("forwards");
    #endif
    reset();
    maps[0] = parseImg(imageInt
    #if PROFILING
        ,parentTimer?forwardsTimer:nullptr
    #endif
    );
    normaliseImg(maps[0]
    #if PROFILING
        ,parentTimer?forwardsTimer:nullptr
    #endif
    );
    #if PROFILING
        Timer *convolutionalLayersTimer = nullptr;
        if(parentTimer) convolutionalLayersTimer = forwardsTimer->addChildTimer("convolutionalLayers");
    #endif
    //Convolutional and pooling layers
    for(int l=1;l<mapDimens.size();l++){
        #if PROFILING
            Timer *convolutionalLayerTimer = nullptr;
            if(parentTimer) convolutionalLayerTimer = convolutionalLayersTimer->addChildTimer("convolutionLayer"+std::to_string(l-1));
        #endif
        for(int i=0;i<mapDimens[l].c;i++){
            //Does copy-elision and so no ctor is called and memory is shared
            Tensor currChannel = maps[l].slice({i}); 
            if(kernelSizes[l-1].first==0 || kernelSizes[l-1].second==0){
                //1:1 mapping for a max pool layer
                Tensor prevChannel = maps[l-1].slice({i});
                currChannel = maxPool(prevChannel,strides[l-1].second,strides[l-1].first); //maxPool requires 1:1 channels between layers
            }
            else{   
                //Slice with biases
                Tensor kernel = kernels[l-1].slice({i},{i});
                if(i==0){
                    //We need to set the paddedMap with the correct data and padding 
                    currChannel = convolution(maps[l-1],paddedMaps[l-1],kernel,strides[l-1].second,strides[l-1].first
                    #if PROFILING
                        ,parentTimer?convolutionalLayerTimer:nullptr
                    #endif
                    );
                }   
                else{
                    //We've already set the paddedMap with the correct data
                    //Tell it not to pad
                    currChannel = convolution(paddedMaps[l-1],kernel,strides[l-1].second,strides[l-1].first,false
                    #if PROFILING
                        ,parentTimer?convolutionalLayerTimer:nullptr
                    #endif
                    );
                }
            }
        }
        #if PROFILING
            if(parentTimer) convolutionalLayerTimer->stop();
        #endif
    }
    #if PROFILING
        Timer *poolingTimer;
        if(parentTimer){
            convolutionalLayersTimer->stop();
            poolingTimer = forwardsTimer->addChildTimer("pooling");
        }
    #endif
    //Final pooling 
    int poolingDimenX = mapDimens[mapDimens.size()-1].w/strides[strides.size()-1].second;
    int poolingDimenY = mapDimens[mapDimens.size()-1].h/strides[strides.size()-1].first;
    int poolingArea = poolingDimenY*poolingDimenX;
    Tensor pooled({mapDimens[mapDimens.size()-1].c,poolingDimenY,poolingDimenX});
    float *pooledData = pooled.getData();
    float *activations0Data = activations[0].getData();
    std::vector<int> pooledChildSizes = pooled.getChildSizes();
    for(int i=0;i<mapDimens[mapDimens.size()-1].c;i++){
        Tensor pooledChannel = pooled.slice({i});
        Tensor prevChannel = maps[mapDimens.size()-1].slice({i});
        int *maxPoolIndicesMap = &(maxPoolIndices[maxPoolIndices.size()-1][i*poolingArea]);
        pooledChannel = maxPool(prevChannel,strides[strides.size()-1].second,strides[strides.size()-1].first,maxPoolIndicesMap);
        int activationsPoolingArea = i*poolingArea;
        int poolingChannel = i*pooledChildSizes[0];
        for(int y=0;y<poolingDimenY;y++){
            int activationsPoolingRow = activationsPoolingArea + y*poolingDimenX;
            int poolingRow = poolingChannel + y*pooledChildSizes[1];
            std::memcpy(
                activations0Data+activationsPoolingRow,
                pooledData+poolingRow,
                poolingDimenX*sizeof(float)
            );
        }
    }
    #if PROFILING
        Timer *mlpTimer = nullptr;
        if(parentTimer){
            poolingTimer->stop();
            mlpTimer = forwardsTimer->addChildTimer("mlp");
        }
    #endif
    //MLP


    for(int l=0;l<weights.size();l++){
        float *biasesData = weights[l].getBiases()->getData();
        float* __restrict__ prevActivations = activations[l].getData();
        float* __restrict__ currActivations = activations[l+1].getData();
        float *currWeights = weights[l].getData();
        for(int i=0;i<numNeurons[l+1];i++){
            int weightsTo = i*numNeurons[l];
            int j=0;
            float *currWeightsTo = currWeights + weightsTo;
            for(;j+3<numNeurons[l];j+=4){
                float32x4_t prevActivations128 = vld1q_f32(&prevActivations[j]);
                float32x4_t currWeights128 = vld1q_f32(&currWeightsTo[j]);
                currActivations[i] += dotProduct4f(prevActivations128,currWeights128);
            }
            //scalar tail
            for(;j<numNeurons[l];j++){
                currActivations[i] += prevActivations[j] * currWeightsTo[j]; 
            }
            currActivations[i] += biasesData[i]; //add bias
            if(l!=weights.size()-1){ //Don't ReLU the last layer
                currActivations[i] = leakyRelu(currActivations[i]);
            }
        }
    }
    #if PROFILING
        if(parentTimer) mlpTimer->stop();
    #endif
    #if DEBUG >= 2
        saveMaps();
        saveActivations();
    #endif
    //Sigmoid the hasWeed neuron
    *activations[activations.size()-1][2] = sigmoid(*activations[activations.size()-1][2]);
    //Leave the others 

    std::vector<float> res(numNeurons[numNeurons.size()-1]);
    for(int i=0;i<numNeurons[numNeurons.size()-1];i++){
        res[i] = *(activations[activations.size()-1][i]);
    }
    #if DEBUG
        d1 outputVec = activations[activations.size()-1].toVector<d1>();
        std::cout << "[";
        for(int i=0;i<outputVec.size()-1;i++){
            std::cout << std::to_string(outputVec[i])+",";
        }
        std::cout << std::to_string(outputVec[outputVec.size()-1])+"]" << std::endl;
    #endif
    #if DEBUG >= 2
        saveMaps();
        saveActivations();
    #endif
    
    return res;
} 
