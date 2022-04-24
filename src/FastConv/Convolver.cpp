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
    for (int i = 0; i < m_IRLength - 1; ++i) {
        m_buffer->putPostInc(0.F);
        for (int j = 0; j < m_IRLength; ++j) {
            output[i] += m_buffer->get(m_IRLength-j) * m_IR[j];
        }
        m_buffer->getPostInc();
    }
    applyWetGain(output, m_IRLength - 1);
    return Error_t::kNoError;
}

UniformlyPartitionedFFTConvolver::UniformlyPartitionedFFTConvolver(float* impulseResponse, int irLength, int blockLength) {
    init(impulseResponse, irLength, blockLength);
}

UniformlyPartitionedFFTConvolver::~UniformlyPartitionedFFTConvolver() {
    reset();
}

Error_t UniformlyPartitionedFFTConvolver::init(float* impulseResponse, int irLength, int blockLength) {
    int doubleBlockLength = blockLength << 1;
    m_blockLengthPlusOne = blockLength + 1;

    Error_t ret;
    if ((ret = ConvolverInterface::init(impulseResponse, irLength, blockLength)) != Error_t::kNoError) return ret;

    m_originIRLengthInSample = irLength;

    CFft::createInstance(m_pCFft);
    m_pCFft->initInstance(doubleBlockLength, 1, CFft::kWindowHann, CFft::kNoWindow);

    // calculate the number of blocks of the impulse response
    this->m_blockLength = blockLength;
    m_IRNumBlock = irLength / blockLength;
    if (irLength % blockLength) ++m_IRNumBlock;

    // allocate memory to store the spectrogram of the impulse response
    m_H_real = new float* [m_IRNumBlock];
    m_H_imag = new float* [m_IRNumBlock];

    // initialize buffer for temporary results
    m_buffer = new CRingBuffer<float>((m_IRNumBlock + 1) * blockLength);

    // initialize the temp spaces
    aReal = new float[m_blockLengthPlusOne]();
    cReal = new float[m_blockLengthPlusOne]();
    aImag = new float[m_blockLengthPlusOne]();
    cImag = new float[m_blockLengthPlusOne]();
    temp  = new float[m_blockLengthPlusOne]();
    iFFTTemp = new float[doubleBlockLength]();

    // allocate memory for frequency domain
    m_X = new CFft::complex_t[doubleBlockLength];
    m_X_origin = new CFft::complex_t[doubleBlockLength];

    // temporary variable for blocked IR freq representatie for blocked IR freq representation
    auto m_H = new CFft::complex_t[doubleBlockLength];

    // pre-calculate the spectrogram of the impulse response
    for (int i = 0; i < m_IRNumBlock; ++i) {
        m_H_real[i] = new float[m_blockLengthPlusOne]();
        m_H_imag[i] = new float[m_blockLengthPlusOne]();

        memset(iFFTTemp, 0, sizeof(float) * doubleBlockLength);
        memcpy(iFFTTemp, impulseResponse + (i * blockLength), sizeof(float) * std::min(blockLength, m_originIRLengthInSample - i * blockLength));

        m_pCFft->doFft(m_H, iFFTTemp);
        m_pCFft->splitRealImag(m_H_real[i], m_H_imag[i], m_H);
    }

    delete[] m_H;

    return Error_t::kNoError;
}

Error_t UniformlyPartitionedFFTConvolver::reset() {
    for (int i = 0; i < m_IRNumBlock; ++i) {
        delete[] m_H_real[i];
        delete[] m_H_imag[i];
    }
    delete[] m_H_real;
    delete[] m_H_imag;
    delete[] m_X;

    delete[] aReal;
    delete[] cReal;
    delete[] aImag;
    delete[] cImag;

    delete[] iFFTTemp;
    delete[] temp;

    delete m_buffer;

    m_pCFft->resetInstance();
    CFft::destroyInstance(m_pCFft);
    m_pCFft = nullptr;

    return Error_t::kNoError;
}

Error_t UniformlyPartitionedFFTConvolver::process(float* output, const float* input, int bufferLength) {
    int accuLength = 0;
    int curBufferLength;
    while (bufferLength) {
        curBufferLength = std::min(bufferLength, m_blockLength);
        __processOneBlock(output + accuLength, input + accuLength, curBufferLength);
        accuLength += curBufferLength;
        bufferLength -= curBufferLength;
    }
    return Error_t::kNoError;
}

void UniformlyPartitionedFFTConvolver::__processOneBlock(float* output, const float* input, int bufferLength) {
    assert(bufferLength <= m_blockLength);
    int doubleBlockLength = m_blockLength << 1;
    memset(iFFTTemp + bufferLength, 0, sizeof(float) * (doubleBlockLength - bufferLength));
    memcpy(iFFTTemp, input, sizeof(float) * bufferLength);
    m_pCFft->doFft(m_X_origin, iFFTTemp);

    // do convolution for each block on frequency domain
    for (int i = 0; i < m_IRNumBlock; ++i) {
        memcpy(m_X, m_X_origin, sizeof(CFft::complex_t) * doubleBlockLength);
        __complexVectorMul_I(m_X, m_H_real[i], m_H_imag[i]); // Y[i] = X[i] * H[i], but &Y == &X
        m_pCFft->doInvFft(iFFTTemp, m_X);
        CVectorFloat::mulC_I(iFFTTemp, static_cast<float>(doubleBlockLength), doubleBlockLength);

        int offset = i * m_blockLength;
        for (int j = 0; j < 2; ++j) {
            __addToRingBuffer(m_buffer->getHead(offset + j * m_blockLength), iFFTTemp + j * m_blockLength, m_blockLength);
        }
    }

    // update output using ringbuffer buffer
    __flushRingBufferToOutput(output, bufferLength);

    applyWetGain(output, bufferLength);
}

Error_t UniformlyPartitionedFFTConvolver::flushBuffer(float *pfOutputBuffer) {
    int numSampleToCopy;
    int numSampleLeftAfterBufferHead;

    for (int i = 0; i < m_originIRLengthInSample - 1; ) {
        numSampleToCopy = std::min(m_blockLength, m_originIRLengthInSample - 1 - i);
        if (!numSampleToCopy) break;

        numSampleLeftAfterBufferHead = m_buffer->getLength() - (m_buffer->getHead(i) - m_buffer->begin());

        if (numSampleLeftAfterBufferHead < numSampleToCopy) {
            memcpy(pfOutputBuffer + i, m_buffer->getHead(i), sizeof(float) * numSampleLeftAfterBufferHead);
            numSampleToCopy -= numSampleLeftAfterBufferHead;
            i += numSampleLeftAfterBufferHead;
        }

        memcpy(pfOutputBuffer + i, m_buffer->getHead(i), sizeof(float) * numSampleToCopy);
        i += numSampleToCopy;
    }
    applyWetGain(pfOutputBuffer, m_originIRLengthInSample - 1);
    return Error_t::kNoError;
}

void UniformlyPartitionedFFTConvolver::__complexVectorMul_I(CFft::complex_t* A, const float* bReal, const float* bImag) {
    m_pCFft->splitRealImag(aReal, aImag, A);

    __complexVectorMul_I(aReal, aImag, bReal, bImag);

    m_pCFft->mergeRealImag(A, cReal, cImag);
}

void UniformlyPartitionedFFTConvolver::__complexVectorMul_I(const float* aReal, const float* aImag, const float *bReal, const float* bImag) {
    // check https://mathworld.wolfram.com/ComplexMultiplication.html for how to reduce 1 multiplication

    // cImag = a + b
    // cReal = c + d
    // cImag *= cReal -> (a + b) * (c + d)
    CVectorFloat::copy(cImag, aReal,  m_blockLengthPlusOne);
    CVectorFloat::add_I(cImag, aImag, m_blockLengthPlusOne);
    CVectorFloat::copy(cReal, bReal,  m_blockLengthPlusOne);
    CVectorFloat::add_I(cReal, bImag, m_blockLengthPlusOne);
    CVectorFloat::mul_I(cImag, cReal, m_blockLengthPlusOne);

    // cReal = ac
    CVectorFloat::copy(cReal, aReal,  m_blockLengthPlusOne);
    CVectorFloat::mul_I(cReal, bReal, m_blockLengthPlusOne);

    // cImag -= ac -> (a + b) * (c + d) - ac
    CVectorFloat::sub_I(cImag, cReal, m_blockLengthPlusOne);

    // temp = bd
    CVectorFloat::copy(temp, aImag,   m_blockLengthPlusOne);
    CVectorFloat::mul_I(temp, bImag,  m_blockLengthPlusOne);

    // cReal -= temp -> ac - bd
    CVectorFloat::sub_I(cReal, temp,  m_blockLengthPlusOne);

    // cImag -= temp -> (a + b) * (c + d) - ac - bd
    CVectorFloat::sub_I(cImag, temp,  m_blockLengthPlusOne);

}

void UniformlyPartitionedFFTConvolver::__addToRingBuffer(float* bufferHead, float* data, int length) {
    auto headIndex = bufferHead - m_buffer->begin();
    if (headIndex + length >= m_buffer->getLength()) {
        auto restLength = (m_buffer->getLength() - headIndex);
        CVectorFloat::add_I(bufferHead, data, restLength);

        length -= restLength;
        data += restLength;
        bufferHead = m_buffer->getHead(0);
    }

    CVectorFloat::add_I(bufferHead, data, length);
}

void UniformlyPartitionedFFTConvolver::__flushRingBufferToOutput(float* output, int length) {
    // notice that bufferLength is not essentially blockLength... (caused by input block with 
    // length not divisible by blockLength) thus the head is not always located at the 
    // beginning of the block in buffer. We need to deal with that.

    auto headIndex = m_buffer->getReadIdx();

    // overflow, we need to cpy & set two memory region
    if (headIndex + length >= m_buffer->getLength()) {
        auto restLength = (m_buffer->getLength() - headIndex);
        __flushRingBufferToOutputWithNoCheck(output, restLength);
        length -= restLength;
    }

    __flushRingBufferToOutputWithNoCheck(output, length);
}

void UniformlyPartitionedFFTConvolver::__flushRingBufferToOutputWithNoCheck(float* output, int length) {
    auto head = m_buffer->getHead(0);
    memcpy(output, head, sizeof(float) * length);
    memset(head, 0, sizeof(float) * length);
    m_buffer->setReadIdx(m_buffer->getReadIdx() + length);
}

