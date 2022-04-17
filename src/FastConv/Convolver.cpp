#include "Convolver.h"

ConvolverInterface::ConvolverInterface() {
    reset();
}

ConvolverInterface::~ConvolverInterface() {
    reset();
}

Error_t ConvolverInterface::init(float* impulseResponse, int irLength, int blockLength) {
    m_IR = impulseResponse;
    m_IRLength = irLength;
    m_blockLength = blockLength;
    return Error_t::kNoError;
}

Error_t ConvolverInterface::reset() {
    m_IRLength = 0;
    m_blockLength = 0;
    return Error_t::kNoError;
}

Error_t ConvolverInterface::setWetGain(float wetGain) {
    m_wetGain = wetGain;
    return Error_t::kNoError;
}

ConvolverInterface* ConvolverFactory::createConvolver(float* impulseResponse, int irLength, int blockLength, CFastConv::ConvCompMode_t choice) {
    switch (choice)
    {
    case CFastConv::kTimeDomain:
        return new TrivialFIRConvolver(impulseResponse, irLength, blockLength);
    case CFastConv::kFreqDomain:
        return new UniformlyPartitionedFFTConvolver(impulseResponse, irLength, blockLength);
    case CFastConv::kNumConvCompModes:
        break;
    default:
        break;
    }
    // throw new std::exception("Invalid parameter of convolver type choice");
}

TrivialFIRConvolver::TrivialFIRConvolver(float* impulseResponse, int irLength, int blockLength) {
    init(impulseResponse, irLength, blockLength);
}

TrivialFIRConvolver::~TrivialFIRConvolver() { reset(); }

Error_t TrivialFIRConvolver::initBuffer(int bufferLength) {
    // init buffer
    m_buffer = new CRingBuffer<float>(bufferLength);

    // check if buffer is correctly inited
    if (m_buffer == nullptr) {
        return Error_t::kUnknownError;
    }
    m_buffer->setReadIdx(0);
    m_buffer->setWriteIdx(0);
    return Error_t::kNoError;
}

Error_t TrivialFIRConvolver::deleteBuffer() {
    if (m_buffer != nullptr) {
        delete m_buffer;
    }
    m_buffer = nullptr;
    return Error_t::kNoError;
}

Error_t TrivialFIRConvolver::init(float* impulseResponse, int irLength, int blockLength) {
    Error_t ret;
    if ((ret = ConvolverInterface::init(impulseResponse, irLength, blockLength)) != Error_t::kNoError) return ret;
    if ((ret = initBuffer(irLength)) != Error_t::kNoError) return ret;
    return Error_t::kNoError;
}

Error_t TrivialFIRConvolver::reset() {
    Error_t ret;
    if ((ret = deleteBuffer()) != Error_t::kNoError) return ret;
    return Error_t::kNoError;
}

Error_t TrivialFIRConvolver::process(float* output, const float* input, int bufferLength) {
    for (int i = 0; i < bufferLength; ++i) {
        m_buffer->putPostInc(input[i]);
        for (int j = 0; j < m_IRLength; ++j) {
            output[i] += m_buffer->get(m_IRLength-j) * (m_IR[j] * m_wetGain);
        }
        m_buffer->getPostInc();
    }
    return Error_t::kNoError;
}

Error_t TrivialFIRConvolver::flushBuffer(float* output) {
    for (int i = 0; i < m_IRLength; ++i) {
        m_buffer->putPostInc(0.F);
        for (int j = 0; j < m_IRLength; ++j) {
            output[i] += m_buffer->get(m_IRLength-j) * (m_IR[j] * m_wetGain);
        }
        m_buffer->getPostInc();
    }
    return Error_t::kNoError;
}

UniformlyPartitionedFFTConvolver::UniformlyPartitionedFFTConvolver(float* impulseResponse, int irLength, int blockLength) {
    init(impulseResponse, irLength, blockLength);
}

UniformlyPartitionedFFTConvolver::~UniformlyPartitionedFFTConvolver() {
    // reset();
    // pCFft->resetInstance();
    // CFft::destroyInstance(pCFft);
    // pCFft = nullptr;
}

Error_t UniformlyPartitionedFFTConvolver::init(float* impulseResponse, int irLength, int blockLength) {
    Error_t ret;
    if ((ret = ConvolverInterface::init(impulseResponse, irLength, blockLength)) != Error_t::kNoError) return ret;

    CFft::createInstance(pCFft);
    pCFft->initInstance(2 * blockLength, 1, CFft::kWindowHann, CFft::kNoWindow);

    // calculate the number of blocks of the impulse response
    this->blockLength = blockLength;
    m_IRNumBlock = irLength / blockLength;
    if (irLength % blockLength) ++m_IRNumBlock;

    // allocate memory to store the spectrogram of the impulse response
    IR_Freq = new CFft::complex_t* [m_IRNumBlock];

    // initialize buffer for temporary results
    bufferReal = new CRingBuffer<CFft::complex_t>((m_IRNumBlock + 1) * blockLength);
    bufferImag = new CRingBuffer<CFft::complex_t>((m_IRNumBlock + 1) * blockLength);

    // pre-calculate the spectrogram of the impulse response
    for (int i = 0; i < m_IRNumBlock; ++i) {
        IR_Freq[i] = new CFft::complex_t[blockLength * 2];
        pCFft->doFft(IR_Freq[i], impulseResponse + (i * blockLength));
    }

    // initialize the temp spaces
    aReal = new float[blockLength + 1]; memset(aReal, 0, sizeof(float) * (blockLength + 1));
    bReal = new float[blockLength + 1]; memset(bReal, 0, sizeof(float) * (blockLength + 1));
    cReal = new float[blockLength + 1]; memset(cReal, 0, sizeof(float) * (blockLength + 1));
    aImag = new float[blockLength + 1]; memset(aImag, 0, sizeof(float) * (blockLength + 1));
    bImag = new float[blockLength + 1]; memset(bImag, 0, sizeof(float) * (blockLength + 1));
    cImag = new float[blockLength + 1]; memset(cImag, 0, sizeof(float) * (blockLength + 1));
    temp  = new float[blockLength + 1]; memset(temp,  0, sizeof(float) * (blockLength + 1));
    iFFTTemp = new float[blockLength * 2]; memset(iFFTTemp,  0, sizeof(float) * (blockLength * 2));

    xFreq = new CFft::complex_t[blockLength * 2];

    return Error_t::kNoError;
}

Error_t UniformlyPartitionedFFTConvolver::reset() {
    for (int i = 0; i < m_IRNumBlock; ++i) delete[] IR_Freq[i];
    delete[] IR_Freq;
    delete[] aReal;
    delete[] bReal;
    delete[] cReal;
    delete[] aImag;
    delete[] bImag;
    delete[] cImag;
    delete[] xFreq;
    delete[] iFFTTemp;
    delete[] temp;
    delete bufferReal;
    delete bufferImag;
    return Error_t::kNoError;
}

Error_t UniformlyPartitionedFFTConvolver::process(float* output, const float* input, int bufferLength) {
    memset(iFFTTemp, 0, sizeof(float) * (blockLength * 2));
    memcpy(iFFTTemp, input, sizeof(float) * bufferLength);
    pCFft->doFft(xFreq, iFFTTemp);

    // do convolution for each block on frequency domain
    // for (int i = 0; i < m_IRNumBlock; ++i) {
    //     __complexVectorMul_I(xFreq, IR_Freq[i]); // Y[i] = X[i] * H[i]
    //     int offset = i * blockLength;
    //     for (int j = 0; j < 2; ++j) {
    //         float* currentBlockHeadReal = bufferReal->getHead(offset + j * blockLength);
    //         float* currentBlockHeadImag = bufferImag->getHead(offset + j * blockLength);
    //         CVectorFloat::add_I(currentBlockHeadReal, cReal + j * blockLength, blockLength);
    //         CVectorFloat::add_I(currentBlockHeadImag, cImag + j * blockLength, blockLength);
    //     }
    // }

    // // ringbuffer is updated thus xFreq can be overwritten
    // float* currentBlockHeadReal = bufferReal->getHead(0);
    // float* currentBlockHeadImag = bufferImag->getHead(0);
    // pCFft->mergeRealImag(xFreq, currentBlockHeadReal, currentBlockHeadImag);

    pCFft->doInvFft(iFFTTemp, xFreq);
    memcpy(output, iFFTTemp, sizeof(float) * bufferLength);

    // update read index for each buffer
    bufferReal->setReadIdx(bufferReal->getReadIdx() + blockLength);
    bufferImag->setReadIdx(bufferImag->getReadIdx() + blockLength);

    return Error_t::kNoError;
}

Error_t UniformlyPartitionedFFTConvolver::flushBuffer(float *pfOutputBuffer) {
    // TODO write flush buffer procedure
    float* zeros = new float[blockLength];
    memset(zeros, 0, sizeof(float) * blockLength);
    for (int i = 0; i < 5; ++i) {
        process(pfOutputBuffer + i * blockLength, zeros, blockLength);
    }
    delete[] zeros;
    return Error_t::kUnknownError;
}

void UniformlyPartitionedFFTConvolver::__complexVectorMul_I(CFft::complex_t* a, const CFft::complex_t* b) {
    pCFft->splitRealImag(aReal, aImag, a);
    pCFft->splitRealImag(bReal, bImag, b);

    // c_r = a_r * b_r - a_i * b_i
    CVectorFloat::copy(cReal, aReal, blockLength); 
    CVectorFloat::mul_I(cReal, bReal, blockLength);
    CVectorFloat::copy(cImag, aImag, blockLength);
    CVectorFloat::mul_I(cImag, bImag, blockLength);
    CVectorFloat::sub_I(cReal, cImag, blockLength);
    
    // c_i = a_r * b_i + a_i * b_r
    CVectorFloat::copy(cImag, aReal, blockLength);
    CVectorFloat::mul_I(cImag, bImag, blockLength);
    CVectorFloat::copy(temp, aImag, blockLength);
    CVectorFloat::mul_I(temp, bReal, blockLength);
    CVectorFloat::add_I(cImag, temp, blockLength);
}

