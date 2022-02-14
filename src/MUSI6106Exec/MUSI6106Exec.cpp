
#include <iostream>
#include <ctime>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "CombFilterIf.h"
#include "RingBuffer.h"

using std::cout;
using std::endl;

// local function declarations
void    showClInfo ();

typedef struct CombFilterArgs {
    CCombFilterIf::CombFilterType_t filterType = CCombFilterIf::CombFilterType_t::kCombFIR;
    float delay = 0.1F;
    float gain = 0.5F;
    std::string inputPath = "fake_id.wav";
    std::string outputPath = "out.wav";
} CombFilterArgs_t;

/*
Simple command line argument parser.
*/
CombFilterArgs_t parseArg(int argc, char** argv) {
    CombFilterArgs_t result;
    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') { /* this is an argument */
            if (argv[i][1] == 't') {
                if (i == argc - 1) throw(std::exception("Incomplete argument of comb filter type!"));
                ++i;
                if (!strcmp(argv[i], "FIR")) { /* argv[i][1:] == "FIR" */
                    result.filterType = CCombFilterIf::CombFilterType_t::kCombFIR;
                }
                else if (!strcmp(argv[i], "IIR")) {
                    result.filterType = CCombFilterIf::CombFilterType_t::kCombIIR;
                }
                else {
                    throw(std::exception("unknown argument!"));
                }
            }
            else if (argv[i][1] == 'd') {
                if (i == argc - 1) throw(std::exception("Incomplete argument of delay!"));
                result.delay = std::stof(argv[++i]);
            }
            else if (argv[i][1] == 'g') {
                if (i == argc - 1) throw(std::exception("Incomplete argument of gain!"));
                result.gain = std::stof(argv[++i]);
            }
            else if (argv[i][1] == 'i') {
                if (i == argc - 1) throw(std::exception("Incomplete argument of input file path!"));
                result.inputPath = std::string(argv[++i]);
            }
            else if (argv[i][1] == 'o') {
                if (i == argc - 1) throw(std::exception("Incomplete argument of output file path!"));
                result.outputPath = std::string(argv[++i]);
            }
            else {
                throw(std::exception("unknown argument!"));
            }
        }
    }
    return result;
}

void processFile(CombFilterArgs_t& args) {
    static const int kBlockSize = 1024;

    clock_t time = 0;

    /* Initialize input & output audio files */
    CAudioFileIf *phInputAudioFile = nullptr;
    CAudioFileIf *phOutputAudioFile = nullptr;

    CAudioFileIf::FileSpec_t stInputFileSpec;
    CAudioFileIf::FileSpec_t stOutputFileSpec;

    CAudioFileIf::create(phInputAudioFile);
    CAudioFileIf::create(phOutputAudioFile);

    phInputAudioFile->openFile(args.inputPath, CAudioFileIf::FileIoType_t::kFileRead);
    phInputAudioFile->getFileSpec(stInputFileSpec);
    phOutputAudioFile->openFile(args.outputPath, CAudioFileIf::FileIoType_t::kFileWrite, &stInputFileSpec);

    /* Initialize combfilter */
    CCombFilterIf* combFilterInterface = nullptr;
    CCombFilterIf::create(combFilterInterface);
    combFilterInterface->init(args.filterType, args.delay, stInputFileSpec.fSampleRateInHz, stInputFileSpec.iNumChannels);
    combFilterInterface->setParam(CCombFilterIf::FilterParam_t::kParamDelay, args.delay);
    combFilterInterface->setParam(CCombFilterIf::FilterParam_t::kParamGain, args.gain);

    /* Initialize audio buffer arrays */
    long long kllBlockSize = static_cast<long long>(kBlockSize);

    float** ppfInputAudioData, **ppfOutputAudioData;
    ppfInputAudioData = new float* [stInputFileSpec.iNumChannels];
    ppfOutputAudioData = new float* [stInputFileSpec.iNumChannels];
    for (int i = 0; i < stInputFileSpec.iNumChannels; ++i) {
        ppfInputAudioData[i] = new float[kllBlockSize];
        ppfOutputAudioData[i] = new float[kllBlockSize];
    }

    long long audioLengthInFrame = 0;

    /* Iterate over whole file and apply effect */
    phInputAudioFile->getLength(audioLengthInFrame);
    for (long long pos = 0; pos < audioLengthInFrame; phInputAudioFile->getPosition(pos)) {
        phInputAudioFile->readData(ppfInputAudioData, kllBlockSize);
        combFilterInterface->process(ppfInputAudioData, ppfOutputAudioData, kllBlockSize);
        phOutputAudioFile->writeData(ppfOutputAudioData, kllBlockSize);
    }

    phOutputAudioFile->closeFile();

    /* Clean up allocated things */
    for (int i = 0; i < stInputFileSpec.iNumChannels; ++i) {
        delete [] ppfInputAudioData[i];
        delete [] ppfOutputAudioData[i];
    }
    delete[] ppfInputAudioData;
    delete[] ppfOutputAudioData;
    
    CCombFilterIf::destroy(combFilterInterface);

    CAudioFileIf::destroy(phInputAudioFile);
    CAudioFileIf::destroy(phOutputAudioFile);
}

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
    /* First parse arguments, which is essential for audio file interface & combfilter initialization */
    try {
        CombFilterArgs_t args = parseArg(argc, argv);
        processFile(args);
    }
    catch (std::exception& e) {
        std::fprintf(stderr, "Error when trying to parse arguments: %s", e.what());
    }
    // all done
    return 0;

}


void     showClInfo()
{
    cout << "MUSI6106 Assignment Executable" << endl;
    cout << "(c) 2014-2022 by Alexander Lerch" << endl;
    cout  << endl;

    return;
}
