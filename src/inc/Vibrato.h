/*
Design of choice

We eventually decide not to follow the classic interface-abstract class-implementation structure.

Without interface, the implementation details are exposed, and users have extra work to adapt this specific class.

But the class name suffix Effector indicates that it's an implementation of superclass Effector. Eventually this superclass will become something like juce::AudioProcessor, and natually supports composite mode.

Then, we can design and build more powerful interfaces, including EffectorFactory, EffectorTreePreset, etc. We can of course add create, destroy, init, or reset in higher level definition.

Now there's only one file here for vibrato. Unless we add combfilter into the whole code base and use the same logic to initialize, otherwise there's no need to add tedious functions and make users feel hard to use.

We may include comb filters into the new design though, as their implementation has been merged into this branch.
*/

#if !defined(__Vibrato_hdr__)
#define __Vibrato_hdr__

#include "RingBuffer.h"
#include "Lfo.h"
#include "ErrorDef.h"

class VibratoEffector {
public:
    /*! feedforward or recursive comb filter */
    enum class VibratoParam_t
    {
        kModulationFreq,           //!< finite impulse response filter
        kModulationWidth,           //!< infinite impulse response filter

        kNumVibratoParam
    };

    /*! Vibrato effector constructor
    \param iSampleRate integer sample rate (fs)
    \param iNumChannel number of input channels
    \param fModulationFreqInHz modulation LFO frequency
    \param fModulationWidthInHz the range of vibrato (e.g. +-50Hz)
    */
	VibratoEffector(int iSampleRate, int iNumChannel, float fModulationFreqInHz, float fModulationWidthInHz);

    /*! Vibrato effector destructor
    \param iSampleRate integer sample rate (fs)
    \param fModulationFreqInHz modulation LFO frequency
    \param fModulationWidthInHz the range of vibrato (e.g. +-50Hz)
    */
    ~VibratoEffector();

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

    void reset();

    /*! set LFO shape
    \param arr The array representing the wavetable. Notice there's no boundry check so might generate security issue.
    */
    void setLfoShape(float* arr) {
        if (m_lfo != nullptr) {
            for (int i = 0; i < m_iNumChannel; ++i) {
                if (m_lfo[i] != nullptr) {
                    m_lfo[i]->setWaveTable(arr);
                }
            }
        }
    }

private:
	int m_iSampleRate = 44100;
    int m_iNumChannel = 2;
	float m_fModulationFreqInHz = 0;
	float m_fModulationWidthInS = 0;
    float m_fDelayInSample = 0;

    Error_t setModulationFreq(float fModulationFreqInHz);
    Error_t setModulationWidth(float fModulationWidthInS);

    float getModulationFreq() const;
    float getModulationWidth() const;

	CRingBuffer<float>** m_buffer = nullptr;
    CLfo** m_lfo = nullptr;

    /*! delete buffer pointer if it's not nullptr
    */
    void deleteBuffer();

    /*! update the buffer length with mod freq and mod width. We need both because they both contribute to delay length. The m_buffer will be initialized here.
    */
	void updateDelayBuffer();

    /*! update information for lfo at each channel
    */
    void updateLfo();

    /*! delete all existing lfos
    */
    void deleteLfo();
};

#endif // __Vibrato_hdr__
