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
            m_pCFastConv->reset();
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

            m_pCFastConv->init(IR, IRLength, 32, CFastConv::kFreqDomain);
        }

        virtual void TearDown()
        {
            m_pCFastConv->reset();
            delete[] IR;
        }

        int IRLength = 0;
        float* IR = nullptr;
        CFastConv *m_pCFastConv = nullptr;
    };

    TEST_F(FFTFastConv, identity)
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
            EXPECT_NEAR(IR[i], output[i + shift], 1e-6);
        }

        // also check if the first three are ok
        for (int i = 0; i < shift; ++i) {
            EXPECT_NEAR(output[i], 0, 1e-6);
        }

        delete[] input;
        delete[] output;
    }

    TEST_F(FFTFastConv, flushbuffer) {
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
            EXPECT_NEAR(output[i], IR[i + signalLength - shift], 1e-6);
        }

        delete[] input;
        delete[] output;
    }

    TEST_F(FFTFastConv, varyingBlockSize) {
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
            EXPECT_NEAR(IR[i], output[i + shift], 1e-6);
        }

        // also check if the first three are ok
        for (int i = 0; i < shift; ++i) {
            EXPECT_NEAR(output[i], 0, 1e-6);
        }

        // also check if tails are all zero
        for (int i = IRLength + shift; i < signalLength; ++i) {
            EXPECT_NEAR(output[i], 0, 1e-6);
        }

        delete[] input;
        delete[] output;
    }
}

#endif //WITH_TESTS

