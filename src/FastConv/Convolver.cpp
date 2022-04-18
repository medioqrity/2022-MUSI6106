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

void ConvolverInterface::applyWetGain(float* output, int length) {
    CVectorFloat::mulC_I(output, m_wetGain, length);
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
    return nullptr;
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
            output[i] += m_buffer->get(m_IRLength-j) * m_IR[j];
        }
        m_buffer->getPostInc();
    }
    applyWetGain(output, bufferLength);
    return Error_t::kNoError;
}

Error_t TrivialFIRConvolver::flushBuffer(float* output) {
    for (int i = 0; i < m_IRLength; ++i) {
        m_buffer->putPostInc(0.F);
        for (int j = 0; j < m_IRLength; ++j) {
            output[i] += m_buffer->get(m_IRLength-j) * m_IR[j];
        }
        m_buffer->getPostInc();
    }
    applyWetGain(output, m_IRLength);
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
    int doubleBlockLength = blockLength << 1;

    Error_t ret;
    if ((ret = ConvolverInterface::init(impulseResponse, irLength, blockLength)) != Error_t::kNoError) return ret;

    originIRLengthInSample = irLength;

    CFft::createInstance(pCFft);
    pCFft->initInstance(doubleBlockLength, 1, CFft::kWindowHann, CFft::kNoWindow);

    // calculate the number of blocks of the impulse response
    this->m_blockLength = blockLength;
    m_IRNumBlock = irLength / blockLength;
    if (irLength % blockLength) ++m_IRNumBlock;

    // allocate memory to store the spectrogram of the impulse response
    IR_Freq = new CFft::complex_t* [m_IRNumBlock];

    // initialize buffer for temporary results
    buffer = new CRingBuffer<float>((m_IRNumBlock + 1) * blockLength);

    // initialize the temp spaces
    aReal = new float[blockLength + 1]; memset(aReal, 0, sizeof(float) * (blockLength + 1));
    bReal = new float[blockLength + 1]; memset(bReal, 0, sizeof(float) * (blockLength + 1));
    cReal = new float[blockLength + 1]; memset(cReal, 0, sizeof(float) * (blockLength + 1));
    aImag = new float[blockLength + 1]; memset(aImag, 0, sizeof(float) * (blockLength + 1));
    bImag = new float[blockLength + 1]; memset(bImag, 0, sizeof(float) * (blockLength + 1));
    cImag = new float[blockLength + 1]; memset(cImag, 0, sizeof(float) * (blockLength + 1));
    temp  = new float[blockLength + 1]; memset(temp,  0, sizeof(float) * (blockLength + 1));
    iFFTTemp = new float[doubleBlockLength]; memset(iFFTTemp,  0, sizeof(float) * (doubleBlockLength));

    // allocate memory for frequency domain
    X = new CFft::complex_t[doubleBlockLength];
    X_origin = new CFft::complex_t[doubleBlockLength];

    // pre-calculate the spectrogram of the impulse response
    for (int i = 0; i < m_IRNumBlock; ++i) {
        memset(iFFTTemp, 0, sizeof(float) * doubleBlockLength);
        memcpy(iFFTTemp, impulseResponse + (i * blockLength), sizeof(float) * std::min(blockLength, originIRLengthInSample - i * blockLength));
        IR_Freq[i] = new CFft::complex_t[doubleBlockLength];
        pCFft->doFft(IR_Freq[i], iFFTTemp);
    }

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
    delete[] X;
    delete[] iFFTTemp;
    delete[] temp;
    delete buffer;
    return Error_t::kNoError;
}

Error_t UniformlyPartitionedFFTConvolver::process(float* output, const float* input, int bufferLength) {
    assert(bufferLength <= m_blockLength);
    int doubleBlockLength = m_blockLength << 1;
    memset(iFFTTemp, 0, sizeof(float) * doubleBlockLength);
    memcpy(iFFTTemp, input, sizeof(float) * bufferLength);
    pCFft->doFft(X_origin, iFFTTemp);

    // do convolution for each block on frequency domain
    for (int i = 0; i < m_IRNumBlock; ++i) {
        memcpy(X, X_origin, sizeof(CFft::complex_t) * doubleBlockLength);
        __complexVectorMul_I(X, IR_Freq[i]); // Y[i] = X[i] * H[i]
        pCFft->doInvFft(iFFTTemp, X);
        CVectorFloat::mulC_I(iFFTTemp, static_cast<float>(doubleBlockLength), doubleBlockLength);

        int offset = i * m_blockLength;
        for (int j = 0; j < 2; ++j) {
            float* currentBlockHead = buffer->getHead(offset + j * m_blockLength);
            CVectorFloat::add_I(currentBlockHead, iFFTTemp + j * m_blockLength, m_blockLength);
        }
    }

    // update output using ringbuffer buffer
    // notice that bufferLength is not essentially blockLength... (caused by input block with 
    // length not divisible by blockLength) thus the head is not always located at the 
    // beginning of the block in buffer. We need to deal with that.
    auto head = buffer->getHead(0);
    auto index = head - buffer->begin();

    // overflow, we need to cpy & set two memory region
    if (index + bufferLength >= buffer->getLength()) {
        // index + bufferLength < getLength() + blockLength
        auto restLength = (buffer->getLength() - index);
        memcpy(output, head, sizeof(float) * restLength);
        memset(head, 0, sizeof(float) * restLength);
        buffer->setReadIdx(buffer->getReadIdx() + restLength);
        head = buffer->begin();
        bufferLength -= static_cast<int>(restLength);
    }
    
    // now we are confident that index + bufferLength < buffer->getLength()
    memcpy(output, head, sizeof(float) * bufferLength);
    memset(head, 0, sizeof(float) * bufferLength);

    // update read index for each buffer
    buffer->setReadIdx(buffer->getReadIdx() + bufferLength);

    applyWetGain(output, bufferLength);

    return Error_t::kNoError;
}

Error_t UniformlyPartitionedFFTConvolver::flushBuffer(float *pfOutputBuffer) {
    for (int i = 0; i < m_IRNumBlock; ++i) {
        memcpy(
            pfOutputBuffer + i * m_blockLength, 
            buffer->getHead(i * m_blockLength), 
            sizeof(float) * std::min(m_blockLength, originIRLengthInSample - i * m_blockLength)
        );
    }
    return Error_t::kNoError;
}

void UniformlyPartitionedFFTConvolver::__complexVectorMul_I(CFft::complex_t* a, const CFft::complex_t* b) {
    pCFft->splitRealImag(aReal, aImag, a);
    pCFft->splitRealImag(bReal, bImag, b);

    // c_r = a_r * b_r - a_i * b_i
    CVectorFloat::copy(cReal, aReal,  m_blockLength + 1); 
    CVectorFloat::mul_I(cReal, bReal, m_blockLength + 1);
    CVectorFloat::copy(cImag, aImag,  m_blockLength + 1);
    CVectorFloat::mul_I(cImag, bImag, m_blockLength + 1);
    CVectorFloat::sub_I(cReal, cImag, m_blockLength + 1);
    
    // c_i = a_r * b_i + a_i * b_r
    CVectorFloat::copy(cImag, aReal,  m_blockLength + 1);
    CVectorFloat::mul_I(cImag, bImag, m_blockLength + 1);
    CVectorFloat::copy(temp, aImag,   m_blockLength + 1);
    CVectorFloat::mul_I(temp, bReal,  m_blockLength + 1);
    CVectorFloat::add_I(cImag, temp,  m_blockLength + 1);

    pCFft->mergeRealImag(a, cReal, cImag);
}

