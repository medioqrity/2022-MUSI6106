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
    if (m_IR != nullptr) {
        delete[] m_IR;
    }
    m_IR = nullptr;

    m_IRLength = 0;
    m_blockLength = 0;
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
    throw new std::exception("Invalid parameter of convolver type choice");
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
    if ((ret = ConvolverInterface::reset()) != Error_t::kNoError) return ret;
    if ((ret = deleteBuffer()) != Error_t::kNoError) return ret;
    return Error_t::kNoError;
}

Error_t TrivialFIRConvolver::process(float* output, const float* input, int bufferLength) {
    for (int i = 0; i < bufferLength; ++i) {
        m_buffer->getPostInc();
        m_buffer->putPostInc(input[i]);
        for (int j = 0; j < m_IRLength; ++i) {
            output[i] += m_buffer->get(m_IRLength-j) * m_IR[j];
        }
    }
    return Error_t::kNoError;
}

Error_t TrivialFIRConvolver::flushBuffer(float* output) {
    for (int i = 0; i < m_IRLength; ++i) {
        m_buffer->getPostInc();
        m_buffer->putPostInc(0.F);
        for (int j = 0; j < m_IRLength; ++i) {
            output[i] += m_buffer->get(m_IRLength-j) * m_IR[j];
        }
    }
    return Error_t::kNoError;
}

