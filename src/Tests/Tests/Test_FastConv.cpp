#include "MUSI6106Config.h"

#ifdef WITH_TESTS
#include "Synthesis.h"

#include "Vector.h"
#include "FastConv.h"

#include "gtest/gtest.h"

#include <random>

namespace fastconv_test {
    void CHECK_ARRAY_CLOSE(float* buffer1, float* buffer2, int iLength, float fTolerance)
    {
        for (int i = 0; i < iLength; i++)
        {
            EXPECT_NEAR(buffer1[i], buffer2[i], fTolerance);
        }
    }

    class TimeDomainConv: public testing::Test
    {
    protected:
        void SetUp() override
        {
            m_pCFastConv = new CFastConv();

            // generate a random IR of length 51 samples
            IRLength = 51;
            IR = new float[IRLength];
            memset(IR, 0, sizeof(float) * IRLength);

            // use mt19937 & distribution for high quality random number generation
            std::mt19937 generator(114514); // 114514 is seed
            std::uniform_real_distribution<float> uniformRealDistribution(-1.F, 1.F);
            for (int i = 0; i < IRLength; ++i) {
                IR[i] = uniformRealDistribution(generator);
            }

            m_pCFastConv->init(IR, IRLength, IRLength, CFastConv::kTimeDomain);
        }

        virtual void TearDown()
        {
            delete m_pCFastConv;
            delete[] IR;
        }

        int IRLength = 0;
        float* IR = nullptr;
        CFastConv *m_pCFastConv = nullptr;
    };

    TEST_F(TimeDomainConv, identity)
    {
        // initialize input signal, which is $\delta[n-3]$
        int shift = 3;
        int signalLength = 10;
        float* input = new float[signalLength]; // make the input signal 10 samples long
        memset(input, 0, sizeof(float) * signalLength); // an impulse as input signal at sample index 3
        input[shift] = 1.F;

        // initialize output signal
        float* output = new float[signalLength];
        memset(output, 0, sizeof(float) * signalLength);

        m_pCFastConv->process(output, input, signalLength);

        // check if the output signal is exactly the same with the impulse response
        for (int i = 0; i + shift < signalLength; ++i) {
            EXPECT_NEAR(IR[i], output[i + shift], 1e-8);
        }

        // also check if the first three are ok
        for (int i = 0; i < shift; ++i) {
            EXPECT_EQ(output[i], 0);
        }

        delete[] input;
        delete[] output;
    }

    TEST_F(TimeDomainConv, flushbuffer) {
        // initialize input signal, which is $\delta[n-3]$
        int shift = 3;
        int signalLength = 10;
        float* input = new float[signalLength]; // make the input signal 10 samples long
        memset(input, 0, sizeof(float) * signalLength); // an impulse as input signal at sample index 3
        input[shift] = 1.F;

        // initialize output signal
        float* output = new float[IRLength];
        memset(output, 0, sizeof(float) * IRLength);

        // process & discard result
        m_pCFastConv->process(output, input, signalLength);
        memset(output, 0, sizeof(float) * IRLength);

        // flush
        m_pCFastConv->flushBuffer(output);

        // check if the tail is correct
        for (int i = 0; i + signalLength - shift < IRLength; ++i) {
            EXPECT_EQ(output[i], IR[i + signalLength - shift]);
        }

        delete[] input;
        delete[] output;
    }

    TEST_F(TimeDomainConv, varyingBlockSize) {
        // initialize input signal, which is $\delta[n-3]$
        int shift = 3;
        int signalLength = 10000;
        float* input = new float[signalLength]; // make the input signal 10 samples long
        memset(input, 0, sizeof(float) * signalLength); // an impulse as input signal at sample index 3
        input[shift] = 1.F;

        // initialize output signal
        float* output = new float[signalLength];
        memset(output, 0, sizeof(float) * signalLength);

        // convolve with varying block size
        int blockLengths[] = {
            1, 13, 1023, 2048, 1, 17, 5000, 1897
        };
        for (int i = 0, accu = 0; i < 8; accu += blockLengths[i++]) {
            m_pCFastConv->process(output + accu, input + accu, blockLengths[i]);
        }

        // check if the output signal is exactly the same with the impulse response
        for (int i = 0; i < IRLength && i + shift < signalLength; ++i) {
            EXPECT_NEAR(IR[i], output[i + shift], 1e-8);
        }

        // also check if the first three are ok
        for (int i = 0; i < shift; ++i) {
            EXPECT_EQ(output[i], 0);
        }

        // also check if tails are all zero
        for (int i = IRLength + shift; i < signalLength; ++i) {
            EXPECT_EQ(output[i], 0);
        }

        delete[] input;
        delete[] output;
    }

    class FFTFastConv: public testing::Test
    {
    protected:
        void SetUp() override
        {
            m_pCFastConv = new CFastConv();

            // generate a random IR of length 51 samples
            IRLength = 51;
            IR = new float[IRLength];
            memset(IR, 0, sizeof(float) * IRLength);

            // use mt19937 & distribution for high quality random number generation
            std::mt19937 generator(114514); // 114514 is seed
            std::uniform_real_distribution<float> uniformRealDistribution(-1.F, 1.F);
            for (int i = 0; i < IRLength; ++i) {
                IR[i] = uniformRealDistribution(generator);
            }
        }

        virtual void TearDown()
        {
            delete m_pCFastConv;
            delete[] IR;
        }

        int IRLength = 0;
        float* IR = nullptr;
        int blocksize;
        CFastConv *m_pCFastConv = nullptr;
    };

    TEST_F(FFTFastConv, identity)
    {
        blocksize = 4;
        m_pCFastConv->init(IR, IRLength, blocksize, CFastConv::kFreqDomain);

        // initialize input signal, which is $\delta[n-3]$
        int shift = 3;
        int signalLength = 10;
        float* input = new float[signalLength](); // make the input signal 10 samples long
        input[shift] = 1.F; // an impulse as input signal at sample index 3

        // initialize output signal
        float* output = new float[signalLength + blocksize]();

        m_pCFastConv->process(output, input, signalLength);

        // check if the output signal is exactly the same with the impulse response
        for (int i = 0; i + shift + blocksize < signalLength; ++i) {
            EXPECT_NEAR(IR[i], output[i + shift + blocksize], 1e-6);
        }

        // also check if the first three are ok
        for (int i = 0; i < shift + blocksize; ++i) {
            EXPECT_NEAR(output[i], 0, 1e-6);
        }

        delete[] input;
        delete[] output;
    }

    TEST_F(FFTFastConv, flushbuffer) {
        blocksize = 4;
        m_pCFastConv->init(IR, IRLength, blocksize, CFastConv::kFreqDomain);

        // initialize input signal, which is $\delta[n-3]$
        int shift = 3;
        int signalLength = 10;
        float* input = new float[signalLength](); // make the input signal 10 samples long
        input[shift] = 1.F; // an impulse as input signal at sample index 3

        // initialize output signal
        float* output = new float[IRLength + blocksize - 1]();

        // process & discard result
        m_pCFastConv->process(output, input, signalLength);
        memset(output, 0, sizeof(float) * (IRLength + blocksize - 1));

        // flush
        m_pCFastConv->flushBuffer(output);

        // check if the tail is correct
        for (int i = 0; i + shift < IRLength - 1 && i + signalLength < IRLength; ++i) {
            EXPECT_NEAR(output[i + blocksize + shift], IR[i + signalLength], 1e-6);
        }

        delete[] input;
        delete[] output;
    }

    TEST_F(FFTFastConv, varyingBlockSize) {
        // initialize input signal, which is $\delta[n-3]$
        int shift = 3;
        int signalLength = 10000;
        float* input = new float[signalLength]();
        input[shift] = 1.F;

        // we are going to use a longer IR to test more strictly
        int longerIRLength = signalLength;
        float* longerIR = new float[longerIRLength]();
        std::mt19937 generator(114514); // 114514 is seed
        std::uniform_real_distribution<float> uniformRealDistribution(-1.F, 1.F);
        for (int i = 0; i < longerIRLength; ++i) {
            longerIR[i] = uniformRealDistribution(generator);
        }

        blocksize = 4096;
        m_pCFastConv->init(longerIR, longerIRLength, blocksize, CFastConv::kFreqDomain);

        // initialize output signal
        int outputLength = signalLength + longerIRLength - 1 + blocksize;
        float* output = new float[outputLength]();

        // convolve with varying block size
        int blockLengths[] = {
            1, 13, 1023, 2048, 1, 17, 5000, 1897
        };
        for (int i = 0, accu = 0; i < 8; accu += blockLengths[i++]) {
            m_pCFastConv->process(output + accu, input + accu, blockLengths[i]);
        }

        m_pCFastConv->flushBuffer(output + signalLength);

        // check if the output signal is exactly the same with the impulse response
        for (int i = 0; i < longerIRLength && i + shift + blocksize < outputLength; ++i) {
            EXPECT_NEAR(longerIR[i], output[i + shift + blocksize], 1e-6);
        }

        // also check if the first blocksize + shift samples are ok
        for (int i = 0; i < shift + blocksize; ++i) {
            EXPECT_NEAR(output[i], 0, 1e-6);
        }

        // also check if tails are all zero
        for (int i = longerIRLength + shift + blocksize; i < outputLength; ++i) {
            EXPECT_NEAR(output[i], 0, 1e-6);
        }

        delete[] input;
        delete[] longerIR;
        delete[] output;
    }

    class AlexFastConv: public testing::Test
    {
    protected:
        void SetUp() override
        {
            m_pfInput = new float[m_iInputLength];
            m_pfIr = new float[m_iIRLength];
            m_pfOutput = new float[m_iInputLength + m_iIRLength];

            CVectorFloat::setZero(m_pfInput, m_iInputLength);
            m_pfInput[0] = 1;

            CSynthesis::generateNoise(m_pfIr, m_iIRLength);
            m_pfIr[0] = 1;

            CVectorFloat::setZero(m_pfOutput, m_iInputLength + m_iIRLength);

            m_pCFastConv = new CFastConv();
        }

        virtual void TearDown()
        {
            m_pCFastConv->reset();
            delete m_pCFastConv;

            delete[] m_pfIr;
            delete[] m_pfOutput;
            delete[] m_pfInput;
        }

        float *m_pfInput = 0;
        float *m_pfIr = 0;
        float *m_pfOutput = 0;

        int m_iInputLength = 83099;
        int m_iIRLength = 60001;

        CFastConv *m_pCFastConv = 0;
    };

    TEST_F(AlexFastConv, Params)
    {
        EXPECT_EQ(false, Error_t::kNoError == m_pCFastConv->init(0, 1));
        EXPECT_EQ(false, Error_t::kNoError == m_pCFastConv->init(m_pfIr, 0));
        EXPECT_EQ(false, Error_t::kNoError == m_pCFastConv->init(m_pfIr, 10, -1));
        EXPECT_EQ(false, Error_t::kNoError == m_pCFastConv->init(m_pfIr, 10, 7));
        EXPECT_EQ(true, Error_t::kNoError == m_pCFastConv->init(m_pfIr, 10, 4));
        EXPECT_EQ(true, Error_t::kNoError == m_pCFastConv->reset());
    }

    TEST_F(AlexFastConv, Impulse)
    {
        // impulse with impulse
        int iBlockLength = 4;
        m_pCFastConv->init(m_pfIr, 1, iBlockLength);

        for (auto i = 0; i < 500; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        EXPECT_NEAR(1.F, m_pfOutput[iBlockLength], 1e-6F);
        EXPECT_NEAR(0.F, CVectorFloat::getMin(m_pfOutput, m_iInputLength), 1e-6F);
        EXPECT_NEAR(1.F, CVectorFloat::getMax(m_pfOutput, m_iInputLength), 1e-6F);

        // impulse with dc
        for (auto i = 0; i < 4; i++)
            m_pfOutput[i] = 1;
        iBlockLength = 8;
        m_pCFastConv->init(m_pfOutput, 4, iBlockLength);

        for (auto i = 0; i < 500; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        EXPECT_NEAR(0.F, CVectorFloat::getMean(m_pfOutput, 8), 1e-6F);
        EXPECT_NEAR(1.F, CVectorFloat::getMean(&m_pfOutput[8], 4), 1e-6F);
        EXPECT_NEAR(0.F, CVectorFloat::getMean(&m_pfOutput[12], 400), 1e-6F);

        // impulse with noise
        iBlockLength = 8;
        m_pCFastConv->init(m_pfIr, 27, iBlockLength);

        for (auto i = 0; i < m_iInputLength; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[iBlockLength], 27, 1e-6F);
        CHECK_ARRAY_CLOSE(&m_pfInput[1], &m_pfOutput[iBlockLength + 27], 10, 1e-6F);

        // noise with impulse
        iBlockLength = 8;
        m_pCFastConv->init(m_pfInput, 27, iBlockLength);

        for (auto i = 0; i < m_iIRLength; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfIr[i], 1);

        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[iBlockLength], m_iIRLength - iBlockLength, 1e-6F);
    }
    TEST_F(AlexFastConv, ImpulseTime)
    {
        // impulse with impulse
        int iBlockLength = 4;
        m_pCFastConv->init(m_pfIr, 1, iBlockLength, CFastConv::kTimeDomain);

        for (auto i = 0; i < 500; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        EXPECT_NEAR(1.F, m_pfOutput[0], 1e-6F);
        EXPECT_NEAR(0.F, CVectorFloat::getMin(m_pfOutput, m_iInputLength), 1e-6F);
        EXPECT_NEAR(1.F, CVectorFloat::getMax(m_pfOutput, m_iInputLength), 1e-6F);

        // impulse with dc
        for (auto i = 0; i < 4; i++)
            m_pfOutput[i] = 1;
        iBlockLength = 8;
        m_pCFastConv->init(m_pfOutput, 4, iBlockLength, CFastConv::kTimeDomain);

        for (auto i = 0; i < 500; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        EXPECT_NEAR(1.F, CVectorFloat::getMean(&m_pfOutput[0], 4), 1e-6F);
        EXPECT_NEAR(0.F, CVectorFloat::getMean(&m_pfOutput[4], 400), 1e-6F);

        // impulse with noise
        iBlockLength = 8;
        m_pCFastConv->init(m_pfIr, 27, iBlockLength, CFastConv::kTimeDomain);

        for (auto i = 0; i < m_iInputLength; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[0], 27, 1e-6F);
        CHECK_ARRAY_CLOSE(&m_pfInput[1], &m_pfOutput[27], 10, 1e-6F);

        // noise with impulse
        iBlockLength = 8;
        m_pCFastConv->init(m_pfInput, 27, iBlockLength, CFastConv::kTimeDomain);

        for (auto i = 0; i < m_iIRLength; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfIr[i], 1);

        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[0], m_iIRLength , 1e-6F);
    }

    TEST_F(AlexFastConv, BlockLengths)
    {
        // impulse with noise
        int iBlockLength = 4;

        for (auto j = 0; j < 10; j++)
        {
            m_pCFastConv->init(m_pfIr, 51, iBlockLength);

            for (auto i = 0; i < m_iInputLength; i++)
                m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

            CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[iBlockLength], 51 - iBlockLength, 1e-6F);

            iBlockLength <<= 1;
        }
    }

    TEST_F(AlexFastConv, InputLengths)
    {
        // impulse with noise
        int iBlockLength = 4096;

        int iCurrIdx = 0,
            aiInputLength[] = {
            4095,
            17,
            32157,
            99,
            4097,
            1,
            42723

        };

        m_pCFastConv->init(m_pfIr, m_iIRLength, iBlockLength);

        for (auto i = 0; i < 7; i++)
        {
            m_pCFastConv->process(&m_pfOutput[iCurrIdx], &m_pfInput[iCurrIdx], aiInputLength[i]);
            iCurrIdx += aiInputLength[i];
        }

        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[iBlockLength], m_iIRLength, 1e-6F);
        EXPECT_NEAR(0.F, CVectorFloat::getMean(&m_pfOutput[m_iIRLength + iBlockLength], 10000), 1e-6F);
    }

    TEST_F(AlexFastConv, FlushBuffer)
    {
        // impulse with noise
        int iBlockLength = 8;
        int iIrLength = 27;

        CVectorFloat::setZero(m_pfOutput, m_iInputLength + m_iIRLength);
        m_pCFastConv->init(m_pfIr, iIrLength, iBlockLength);

        m_pCFastConv->process(m_pfOutput, m_pfInput, 1);

        m_pCFastConv->flushBuffer(&m_pfOutput[1]);

        EXPECT_NEAR(0.F, CVectorFloat::getMean(m_pfOutput, iBlockLength), 1e-6F);
        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[iBlockLength], iIrLength, 1e-6F);

        // same for time domain
        CVectorFloat::setZero(m_pfOutput, m_iInputLength + m_iIRLength);
        m_pCFastConv->init(m_pfIr, iIrLength, iBlockLength,CFastConv::kTimeDomain);

        m_pCFastConv->process(m_pfOutput, m_pfInput, 1);

        m_pCFastConv->flushBuffer(&m_pfOutput[1]);

        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[0], iIrLength, 1e-6F);
    }
}

#endif //WITH_TESTS

