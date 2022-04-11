#include "ErrorDef.h"
#include "RingBuffer.h"
#include "FastConv.h"
#include <string>

class ConvolverInterface {
public:
    ConvolverInterface();
    ~ConvolverInterface();
    
    virtual Error_t init(float *impulseResponse, int irLength, int blockLength);
    virtual Error_t reset();

    virtual Error_t process(float* output, const float* input, int bufferLength) = 0;
    virtual Error_t flushBuffer(float *pfOutputBuffer) = 0;

    Error_t setWetGain(float wetGain);

protected:
    float *m_IR;
    int m_IRLength;
    int m_blockLength;
    float m_wetGain = 1.F;
};

class ConvolverFactory {
public:
    static ConvolverInterface* createConvolver(float *impulseResponse, int irLength, int blockLength, CFastConv::ConvCompMode_t choice);
};

class TrivialFIRConvolver: public ConvolverInterface {
public:
    TrivialFIRConvolver(float* impulseResponse, int irLength, int blockLength);
    ~TrivialFIRConvolver();

    Error_t init(float* impulseResponse, int irLength, int blockLength) override;
    Error_t reset() override;

    Error_t process(float* output, const float* input, int bufferLength) override;
    Error_t flushBuffer(float*) override;

private:
    CRingBuffer<float>* m_buffer;

    Error_t initBuffer(int);
    Error_t deleteBuffer();
};


class UniformlyPartitionedFFTConvolver: public ConvolverInterface {
public:
    UniformlyPartitionedFFTConvolver(float* impulseResponse, int irLength, int blockLength);
    ~UniformlyPartitionedFFTConvolver();

    Error_t init(float* impulseResponse, int irLength, int blockLength) override;
    Error_t reset() override;

    Error_t process(float* output, const float* input, int bufferLength) override;
    Error_t flushBuffer(float *pfOutputBuffer) override;
};

