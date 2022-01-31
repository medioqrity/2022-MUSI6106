
// standard headers

// project headers
#include "MUSI6106Config.h"

#include "ErrorDef.h"
#include "Util.h"

#include "CombFilterIf.h"
#include "CombFilter.h"

static const char*  kCMyProjectBuildDate = __DATE__;


CCombFilterIf::CCombFilterIf () :
    m_bIsInitialized(false),
    m_pCCombFilter(0),
    m_fSampleRate(0)
{
    // this should never hurt
    this->reset ();
}


CCombFilterIf::~CCombFilterIf ()
{
    this->reset ();
}

const int  CCombFilterIf::getVersion (const Version_t eVersionIdx)
{
    int iVersion = 0;

    switch (eVersionIdx)
    {
    case kMajor:
        iVersion    = MUSI6106_VERSION_MAJOR; 
        break;
    case kMinor:
        iVersion    = MUSI6106_VERSION_MINOR; 
        break;
    case kPatch:
        iVersion    = MUSI6106_VERSION_PATCH; 
        break;
    case kNumVersionInts:
        iVersion    = -1;
        break;
    }

    return iVersion;
}
const char*  CCombFilterIf::getBuildDate ()
{
    return kCMyProjectBuildDate;
}

Error_t CCombFilterIf::create (CCombFilterIf*& pCCombFilter)
{
    return Error_t::kNoError;
}

Error_t CCombFilterIf::destroy (CCombFilterIf*& pCCombFilter)
{
    return Error_t::kNoError;
}

Error_t CCombFilterIf::init (CombFilterType_t eFilterType, float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels)
{
    // initialize comb filter instance using eFilterType
    if (eFilterType == CombFilterType_t::kCombFIR) {
        m_pCCombFilter = new FIRCombFilter();
    }
    else if (eFilterType == CombFilterType_t::kCombIIR) {
        m_pCCombFilter = new IIRCombFilter();
    }

    else {
        return Error_t::kFunctionInvalidArgsError;
    }
    
    // set parameters
    m_fSampleRate = fSampleRateInHz;

    // the initialization is done
    m_bIsInitialized = true;
}

Error_t CCombFilterIf::reset ()
{
    m_bIsInitialized = false;
    m_fSampleRate = 0;
    
    // comb filter instance is initialized in this class, 
    // therefore this class is responsible for delete.
    // make sure we are not releasing empty pointers.
    if (m_pCCombFilter != nullptr) {
        delete m_pCCombFilter;
    } // what should I do if (m_pCCombFilter == nullptr)?

    return Error_t::kNoError;
}

Error_t CCombFilterIf::process (float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames)
{
    return m_pCCombFilter->process(ppfInputBuffer, ppfOutputBuffer, iNumberOfFrames);
}

Error_t CCombFilterIf::setParam (FilterParam_t eParam, float fParamValue)
{
    return m_pCCombFilter->setParam(eParam, fParamValue);
}

float CCombFilterIf::getParam (FilterParam_t eParam) const
{
    return m_pCCombFilter->getParam(eParam);
}

CCombFilterBase::CCombFilterBase(float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels)
: m_fGain(0.F), m_iNumChannels(iNumChannels), m_buffer(nullptr)
{
    setDelay(fMaxDelayLengthInS);
}

bool CCombFilterBase::isInRange(float lower, float upper, float value) {
    return (lower <= value && value < upper);
}

Error_t CCombFilterBase::setGain(float fGain) {
    if (!isInRange(0.F, kfMaxGain, fGain)) {
        return Error_t::kFunctionInvalidArgsError;
    }
    m_fGain = fGain;
    return Error_t::kNoError;
}

/*! Update the delay length of filter. This would cause deleting & newing CRingBuffer object. 
\param fDelay the delay time IN SECOND
*/
Error_t CCombFilterBase::setDelay(float fDelay) {
    if (!isInRange(0.F, kfMaxDelay, fDelay)) {
        return Error_t::kFunctionInvalidArgsError;
    }
	m_fDelay = fDelay;
	m_iDelayInSample = static_cast<int>(m_fDelay / m_fSampleRateInHz);
	if (m_buffer != nullptr) {
		delete m_buffer;
	}
	m_buffer = new CRingBuffer<float>(m_iDelayInSample);
    return Error_t::kNoError;
}

Error_t CCombFilterBase::setParam(FilterParam_t eParam, float fParamValue) {
    switch (eParam)
    {
    case CCombFilterIf::kParamGain:
        setGain(fParamValue);
        break;
    case CCombFilterIf::kParamDelay:
        setDelay(fParamValue);
        break;
    case CCombFilterIf::kNumFilterParams:
        return Error_t::kFunctionInvalidArgsError;
    default:
        break;
    }
    return Error_t::kNoError;
}

float CCombFilterBase::getGain() const {
    return m_fGain;
}

float CCombFilterBase::getDelay() const {
    return m_fDelay;
}

float CCombFilterBase::getParam(CCombFilterIf::FilterParam_t eParam) const {
    switch (eParam)
    {
    case CCombFilterIf::kParamGain:
        return getGain();
    case CCombFilterIf::kParamDelay:
        return getDelay();
    case CCombFilterIf::kNumFilterParams:
        return -1.F;
    default:
        break;
    }
    return -1.F;
}
