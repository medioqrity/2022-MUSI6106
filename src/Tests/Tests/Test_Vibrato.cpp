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
            m_iLength = 0;
            m_iBlockLength = 1024;

}

#endif //WITH_TESTS
