//
// Created by Rose Sun on 3/2/22.
//

#include "MUSI6106Config.h"

#ifdef WITH_TESTS
#include "Vector.h"
#include "RingBuffer.h"
#include "Synthesis.h"

#include "gtest/gtest.h"

namespace ringBuffer_test {
    void CHECK_ARRAY_CLOSE(float* buffer1, float* buffer2, int iLength, float fTolerance)
    {
        for (int i = 0; i < iLength; i++)
        {
            EXPECT_NEAR(buffer1[i], buffer2[i], fTolerance);
        }
    }

    class RingBufferTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            m_iBufferLength = 20;
            m_pRingBuffer = new CRingBuffer<float> (m_iBufferLength);
            m_pfInput = 0;
            m_pfOutput = 0;
            m_iLength = 100;
            m_iSampleRate = 44100;
            m_pfInput = new float [m_iLength];
            m_pfOutput = new float [m_iLength];
            CSynthesis::generateSine(m_pfInput, 441.F, m_iSampleRate, m_iLength, .6F,0);
        }

        virtual void TearDown()
        {
            delete[] m_pfInput;
            delete m_pRingBuffer;
        }

        CRingBuffer<float> *m_pRingBuffer;
        float *m_pfInput, *m_pfOutput;
        int m_iLength, m_iBufferLength, m_iSampleRate;
    };

    TEST_F(RingBufferTest, interpolation){
        m_pRingBuffer->reset();
        for (int i=0; i<m_iBufferLength; i++){
            m_pRingBuffer->putPostInc(i);
        }
        float f = m_pRingBuffer->get(0.4F);
        EXPECT_EQ(0.4F, f);
    }

    TEST_F(RingBufferTest, putGet){
        m_pRingBuffer->reset();
        for (int i=0; i<m_iLength; i++){
            m_pRingBuffer->putPostInc(m_pfInput[i]);
            m_pfOutput[i] =m_pRingBuffer->getPostInc();
            EXPECT_EQ(m_pRingBuffer->getReadIdx(), m_pRingBuffer->getWriteIdx());
        }
        CHECK_ARRAY_CLOSE(m_pfInput, m_pfOutput,m_iLength, 1e-8);
    }

    TEST_F(RingBufferTest, zeroGet){
        for (int i = 0; i < 114514; ++i) {
            EXPECT_EQ(m_pRingBuffer->get(0.F),m_pRingBuffer->get(0));
            EXPECT_EQ(m_pRingBuffer->get(0),m_pRingBuffer->get());
            EXPECT_EQ(m_pRingBuffer->get(0.F),m_pRingBuffer->get());
            m_pRingBuffer->putPostInc(m_pRingBuffer->getPostInc());
        }
    }

    TEST_F(RingBufferTest, overflow){
        m_pRingBuffer->reset();

    }

}

#endif //WITH_TESTS