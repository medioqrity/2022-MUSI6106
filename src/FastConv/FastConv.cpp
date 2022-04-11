#include "Convolver.h"

CFastConv::CFastConv()
{
    convolver = nullptr;
}

CFastConv::~CFastConv()
{
    reset();
}

Error_t CFastConv::init(float *pfImpulseResponse, int iLengthOfIr, int iBlockLength /*= 8192*/, ConvCompMode_t eCompMode /*= kFreqDomain*/)
{
    convolver = ConvolverFactory::createConvolver(pfImpulseResponse, iLengthOfIr, iBlockLength, eCompMode);
    return Error_t::kNoError;
}

Error_t CFastConv::reset()
{
    if (convolver != nullptr) {
        return convolver->reset();
    }
    return Error_t::kNotInitializedError;
}

Error_t CFastConv::process (float* pfOutputBuffer, const float *pfInputBuffer, int iLengthOfBuffers )
{
    if (convolver != nullptr) {
        return convolver->process(pfOutputBuffer, pfInputBuffer, iLengthOfBuffers);
    }
    return Error_t::kNotInitializedError;
}

Error_t CFastConv::flushBuffer(float* pfOutputBuffer)
{
    if (convolver != nullptr) {
        return convolver->flushBuffer(pfOutputBuffer);
    }
    return Error_t::kNotInitializedError;
}

Error_t CFastConv::setWetGain(float wetGain) {
    return convolver->setWetGain(wetGain);
}
