#include <chrono>
#include <random>
#include <algorithm>

#include "MUSI6106Config.h"

#ifdef WITH_TESTS
#include "Vector.h"
#include "Vibrato.h"

#include "gtest/gtest.h"


namespace vibrato_test {
    void CHECK_ARRAY_CLOSE(float* buffer1, float* buffer2, int iLength, float fTolerance)
    {
        for (int i = 0; i < iLength; i++)
        {
            EXPECT_NEAR(buffer1[i], buffer2[i], fTolerance);
        }
    }

    class VibratoTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            m_pVibrato = 0;
            m_ppfInput = 0;
            m_ppfOutput = 0;
            m_iLength = 1<<16;
            m_iNumChannels = 2;
            m_iBlockLength = 1024;
            m_iSampleRate = 44100;
            m_fModulationFreq = 0;
            m_fModulationWidth = 0;
            m_pVibrato = new VibratoEffector(m_iSampleRate, m_iNumChannels, m_fModulationFreq, m_fModulationWidth);
            m_ppfInput = new float*[m_iNumChannels];
            m_ppfOutput = new float*[m_iNumChannels];
            m_ppfInputTmp = new float*[m_iNumChannels];
            m_ppfOutputTmp = new float*[m_iNumChannels];
            for (int i = 0; i < m_iNumChannels; i++)
            {
                m_ppfInput[i] = new float [m_iLength];
                CSynthesis::generateSine(m_ppfInput[i], 441.F, m_iSampleRate, m_iLength, .6F);
                m_ppfOutput[i] = new float [m_iLength];
                for (int j = 0; j < m_iLength; j++){
                    m_ppfOutput[i][j] = 0;
                }
            }
        }

        virtual void TearDown()
        {
            for (int i = 0; i < m_iNumChannels; i++){
                delete[] m_ppfInput[i];
                delete[] m_ppfOutput[i];
            }
            delete [] m_ppfOutputTmp;
            delete [] m_ppfInputTmp;
            delete [] m_ppfOutput;
            delete [] m_ppfInput;
            delete m_pVibrato;
        }

        void process ()
        {
            int iNumRemainingFrames = m_iLength;
            // put data
            while (iNumRemainingFrames > 0)
            {
                for (int i = 0; i < m_iNumChannels; i++)
                {
                    m_ppfInputTmp[i] = &m_ppfInput[i][m_iLength - iNumRemainingFrames];
                    m_ppfOutputTmp[i] = &m_ppfOutput[i][m_iLength - iNumRemainingFrames];
                }
                int iPutFrames = std::min(m_iBlockLength, iNumRemainingFrames);
                m_pVibrato->process(m_ppfInputTmp, m_ppfOutputTmp, iPutFrames);
                iNumRemainingFrames -= iPutFrames;
            }
        }

        VibratoEffector *m_pVibrato;
        float **m_ppfInput, **m_ppfOutput, **m_ppfInputTmp, **m_ppfOutputTmp;
        float m_fModulationWidth, m_fModulationFreq;
        int m_iLength, m_iBlockLength, m_iNumChannels, m_iSampleRate;
    };

    TEST_F(VibratoTest, setParam){
        // TODO: check negative Freq works
        // TODO: check negative Width works
    }

    TEST_F(VibratoTest, zeroModAmp){
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationFreq, 2);
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationWidth, 0);
        process();
        for (int i=0; i<m_iNumChannels; i++){
            int delay = static_cast<int>(round(m_fModulationWidth * static_cast<float>(m_iSampleRate)));
            CHECK_ARRAY_CLOSE(m_ppfInput[i], m_ppfOutput[i]+delay, m_iLength-delay,1e-8);
        }
    }

    TEST_F(VibratoTest, DCInOut){
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationFreq, 2);
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationWidth, 0.5);
        int delay = m_iSampleRate / 2;
        for (int i=0; i<m_iNumChannels; i++){
            CSynthesis::generateDc(m_ppfInput[i], m_iLength, 0.5);
        }
        process();
        for (int i=0; i<m_iNumChannels; i++){
            CHECK_ARRAY_CLOSE(m_ppfInput[i], m_ppfOutput[i] + delay, m_iLength - delay,1e-8);
        }
    }

    TEST_F(VibratoTest, varyingBlockSize){
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationFreq, 2);
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationWidth, 0.5);

        for (int i=0; i<m_iNumChannels; i++){
            CSynthesis::generateSaw(m_ppfInput[i], 441, 44100, m_iLength);
        }

        // use Mersenne Twister pseudo-random generator to generate safe & high quality random numbers
        std::mt19937 generator(std::chrono::steady_clock::now().time_since_epoch().count());
        int maxBlockSize = m_iLength >> 3;
        for (int offset = 0, blockSize; offset < m_iLength; offset += blockSize) {
            blockSize = std::min(static_cast<int>(generator() % maxBlockSize), m_iLength - offset);
            std::printf("Accumulated offset: %d. Current block size: %d.\n", offset, blockSize);

            for (int i = 0; i < m_iNumChannels; i++)
            {
                m_ppfInputTmp[i] = m_ppfInput[i] + offset;
                m_ppfOutputTmp[i] = m_ppfOutput[i] + offset;
            }
            m_pVibrato->process(m_ppfInputTmp, m_ppfOutputTmp, blockSize);
        }

        // WHOLE BLOCK PROCESS!
        m_pVibrato->reset();
        float** m_ppfOutputRef = new float* [m_iNumChannels];
        for (int i = 0; i < m_iNumChannels; ++i) {
            m_ppfOutputRef[i] = new float[m_iLength];
        }
        m_pVibrato->process(m_ppfInput, m_ppfOutputRef, m_iLength);

        for (int i=0; i<m_iNumChannels; i++){
            CHECK_ARRAY_CLOSE(m_ppfOutput[i], m_ppfOutputRef[i], m_iLength, 1e-8);
        }

        for (int i = 0; i < m_iNumChannels; ++i) {
            delete[] m_ppfOutputRef[i];
        }
        delete[] m_ppfOutputRef;
    }

    TEST_F(VibratoTest, zeroInput){
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationFreq, 2);
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationWidth, 0.5);
        for (int i=0; i<m_iNumChannels; i++){
            CSynthesis::generateDc(m_ppfInput[i], m_iLength, 0);
        }
        process();
        for (int i=0; i<m_iNumChannels; i++){
            CHECK_ARRAY_CLOSE(m_ppfInput[i], m_ppfOutput[i], m_iLength,1e-8);
        }
    }

    TEST_F(VibratoTest, zeroModFreq){
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationFreq, 0);
        m_pVibrato->setParam(VibratoEffector::VibratoParam_t::kModulationWidth, 0);
        for (int i=0; i<m_iNumChannels; i++){
            CSynthesis::generateNoise(m_ppfInput[i], m_iLength);
        }
        process();
        for (int i=0; i<m_iNumChannels; i++){
            CHECK_ARRAY_CLOSE(m_ppfInput[i], m_ppfOutput[i], m_iLength,1e-8);
        }
    }

}

#endif //WITH_TESTS
