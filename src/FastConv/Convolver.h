#include "ErrorDef.h"
#include "RingBuffer.h"
#include "FastConv.h"
#include <string>
#include "Fft.h"
#include "Vector.h"

class ConvolverInterface {
public:
    ConvolverInterface();
    ~ConvolverInterface();
    
    virtual Error_t init(float *impulseResponse, int irLength, int blockLength);
    virtual Error_t reset();

    virtual Error_t process(float* output, const float* input, int bufferLength) = 0;
    virtual Error_t flushBuffer(float *pfOutputBuffer) = 0;

    Error_t setWetGain(float wetGain);
    void applyWetGain(float* output, int length);

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
private:
    int originIRLengthInSample = 0;
    int m_IRNumBlock = 0;
    int m_blockLength = 0;
    CFft* pCFft = nullptr;
    CFft::complex_t* X = nullptr;
    CFft::complex_t* X_origin = nullptr;
    CFft::complex_t** IR_Freq = nullptr; // might be multiple blocks
    CRingBuffer<float>* buffer = nullptr;

    // temporal variables that are useful for calculation
    float* aReal, *bReal, *cReal;
    float* aImag, *bImag, *cImag;
    float* temp, *iFFTTemp;

    void __complexVectorMul_I(CFft::complex_t* a, const CFft::complex_t* b);
};


