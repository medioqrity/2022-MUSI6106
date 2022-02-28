#if !defined(__Vibrato_hdr__)
#define __Vibrato_hdr__

#include "RingBuffer.h"
#include "ErrorDef.h"

class VibratoEffector {
public:
    /*! feedforward or recursive comb filter */
    enum class VibratoParam_t
    {
        kModulationFreq,           //!< finite impulse response filter
        KModulationWidth,           //!< infinite impulse response filter

        kNumVibratoParam
    };

    /*! Vibrato effector constructor
    \param iSampleRate integer sample rate (fs)
    \param fModulationFreqInHz modulation LFO frequency
    \param fModulationWidthInHz the range of vibrato (e.g. +-50Hz)
    */
	VibratoEffector(int iSampleRate, float fModulationFreqInHz, float fModulationWidthInHz);

    /*! sets a comb filter parameter
    \param eParam what parameter (see ::VibratoParam_t)
    \param fParamValue value of the parameter
    \return Error_t
    */
    Error_t setParam (VibratoParam_t eParam, float fParamValue);
    
    /*! return the value of the specified parameter
    \param eParam
    \return float
    */
    float   getParam (VibratoParam_t eParam) const;
    
    /*! processes one block of audio
    \param ppfInputBuffer input buffer [numChannels][iNumberOfFrames]
    \param ppfOutputBuffer output buffer [numChannels][iNumberOfFrames]
    \param iNumberOfFrames buffer length (per channel)
    \return Error_t
    */
    Error_t process (float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames);

private:
	int m_iSampleRate = 44100;
	float m_fModulationFreqInHz = 0;
	float m_fModulationWidthInHz = 0;

	CRingBuffer<float>* m_buffer = nullptr;

    /*! upate the buffer length with mod freq and mod width. We need both because they both contribute to delay length. The m_buffer will be initialized here.
    */
	void updateDelayBuffer();
};

#endif // __Vibrato_hdr__
