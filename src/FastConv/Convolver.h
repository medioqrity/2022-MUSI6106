#include "ErrorDef.h"
#include "RingBuffer.h"
#include "FastConv.h"
#include <string>
#include <memory>
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
    bool m_isInitialized = false;
};

class ConvolverFactory {
public:
    static ConvolverInterface* createConvolver(CFastConv::ConvCompMode_t choice);
    static ConvolverInterface* createConvolver(float *impulseResponse, int irLength, int blockLength, CFastConv::ConvCompMode_t choice);
};

class TrivialFIRConvolver: public ConvolverInterface {
public:
    TrivialFIRConvolver() = default;
    TrivialFIRConvolver(float* impulseResponse, int irLength, int blockLength);
    ~TrivialFIRConvolver();

    Error_t init(float* impulseResponse, int irLength, int blockLength) override;
    Error_t reset() override;

    Error_t process(float* output, const float* input, int bufferLength) override;
    Error_t flushBuffer(float*) override;

private:
    std::unique_ptr<CRingBuffer<float>> m_buffer = nullptr;

    Error_t initBuffer(int);
    Error_t deleteBuffer();
};

class UniformlyPartitionedFFTConvolver: public ConvolverInterface {
public:
    UniformlyPartitionedFFTConvolver() = default;
    UniformlyPartitionedFFTConvolver(float* impulseResponse, int irLength, int blockLength);
    ~UniformlyPartitionedFFTConvolver();

    Error_t init(float* impulseResponse, int irLength, int blockLength) override;
    Error_t reset() override;

    Error_t process(float* output, const float* input, int bufferLength) override;
    Error_t flushBuffer(float *pfOutputBuffer) override;
private:
    int m_originIRLengthInSample = 0;
    int m_IRNumBlock = 0;
    int m_blockLength = 0;
    int m_blockLengthPlusOne = 0;
    CFft* m_pCFft = nullptr;
    CFft::complex_t* m_X = nullptr;
    CFft::complex_t* m_X_origin = nullptr;
    float** m_H_real = nullptr;
    float** m_H_imag = nullptr;
    std::unique_ptr<CRingBuffer<float>> m_inputBuffer = nullptr;
    std::unique_ptr<CRingBuffer<float>> m_outputBuffer = nullptr;

    // temporal variables that are useful for calculation
    float* aReal = nullptr, *cReal = nullptr;
    float* aImag = nullptr, *cImag = nullptr;
    float* temp = nullptr, *iFFTTemp = nullptr;

    void __complexVectorMul_I(const float* aReal, const float* aImag, const float *bReal, const float* bImag);
    void __complexVectorMul_I(CFft::complex_t* A, const float* bReal, const float* bImag);

    void __processOneBlock(float* output, const float* input);
};

