
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
    pCCombFilter = new CCombFilterIf();
    if (pCCombFilter == nullptr) {
        return Error_t::kMemError;
    }
    return Error_t::kNoError;
}

Error_t CCombFilterIf::destroy (CCombFilterIf*& pCCombFilter)
{
    pCCombFilter->reset();
    delete pCCombFilter;
    pCCombFilter = nullptr;
    return Error_t::kNoError;
}

Error_t CCombFilterIf::init (CombFilterType_t eFilterType, float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels)
{
    assert(!m_bIsInitialized);
    // initialize comb filter instance using eFilterType
    if (eFilterType == CombFilterType_t::kCombFIR) {
        m_pCCombFilter = new FIRCombFilter(fMaxDelayLengthInS, fSampleRateInHz, iNumChannels);
    }
    else if (eFilterType == CombFilterType_t::kCombIIR) {
        m_pCCombFilter = new IIRCombFilter(fMaxDelayLengthInS, fSampleRateInHz, iNumChannels);
    }
    else {
        return Error_t::kFunctionInvalidArgsError;
    }
    
    // set parameters
    m_fSampleRate = fSampleRateInHz;

    // the initialization is done
    m_bIsInitialized = true;

    return Error_t::kNoError;
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

    m_pCCombFilter = nullptr;

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

