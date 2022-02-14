
#include <iostream>
#include <ctime>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "CombFilterIf.h"
#include "RingBuffer.h"

#define M_PIl          3.141592653589793238462643383279502884L /* pi */

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

bool test_1_FIR_cancel_out_when_frequency_matches() {
    int iSampleRate = 44100;
    int iNumSample = 1 * iSampleRate;
    int iNumSampleToCycle = 441;
    float** input = new float*[1];
    float** output = new float*[1];
    input[0] = new float[iNumSample];
    output[0] = new float[iNumSample];

    for (int i = 0; i < iNumSample; ++i) {
        input[0][i] = static_cast<float>(sin(2 * M_PIl * static_cast<long double>(i) / static_cast<long double>(iNumSampleToCycle)));
    }

    /* Initialize combfilter */
    CCombFilterIf* combFilterInterface = nullptr;
    CCombFilterIf::create(combFilterInterface);
    combFilterInterface->init(
        CCombFilterIf::CombFilterType_t::kCombFIR, 
        static_cast<float>(iNumSampleToCycle) / static_cast<float>(iSampleRate), 
        static_cast<float>(iSampleRate), 
        1
    );
    combFilterInterface->setParam(CCombFilterIf::FilterParam_t::kParamGain, -1);

    combFilterInterface->process(input, output, iNumSample);

    bool return_value = true;
    for (int i = iNumSampleToCycle; i < iNumSample; ++i) {
        if (output[0][i] > 1e-8F) {
            std::printf("%d: %.8f\n", i, output[0][i]);
            return_value = false;
        }
    }
    
    delete[] input[0];
    delete[] output[0];
    delete[] input;
    delete[] output;
    CCombFilterIf::destroy(combFilterInterface);

    return return_value;
}

bool test_2_IIR_mag_change_when_frequency_matches() {
    int iSampleRate = 44100;
    int iNumSample = 1 * iSampleRate;
    int iNumSampleToCycle = 441;
    float** input = new float*[1];
    float** output = new float*[1];
    input[0] = new float[iNumSample];
    output[0] = new float[iNumSample];

    for (int i = 0; i < iNumSample; ++i) {
        input[0][i] = static_cast<float>(sin(2 * M_PIl * static_cast<long double>(i) / static_cast<long double>(iNumSampleToCycle)));
    }

    /* Initialize combfilter */
    CCombFilterIf* combFilterInterface = nullptr;
    CCombFilterIf::create(combFilterInterface);
    combFilterInterface->init(
        CCombFilterIf::CombFilterType_t::kCombIIR, 
        static_cast<float>(iNumSampleToCycle) / static_cast<float>(iSampleRate), 
        static_cast<float>(iSampleRate), 
        1
    );
    combFilterInterface->setParam(CCombFilterIf::FilterParam_t::kParamGain, 1);

    combFilterInterface->process(input, output, iNumSample);

    bool return_value = true;
    for (int i = iNumSampleToCycle; i < iNumSample && return_value; i += iNumSampleToCycle) {
        int iCycle = i / iNumSampleToCycle;
        for (int j = 0; j < iNumSampleToCycle && return_value; ++j) {
            int k = i + j;
            if (abs(output[0][k - iNumSampleToCycle]) > 1e-5F) {
                if (abs(output[0][k] - output[0][k - iNumSampleToCycle] - input[0][j]) > 1e-5F) {
                    std::printf("%d: %.5f, %d: %.5f, %d: %.5f\n", k, output[0][k], k - iNumSampleToCycle, output[0][k - iNumSampleToCycle], j, input[0][j]);
                    return_value = false;
                }
            }
        }
    }
    
    delete[] input[0];
    delete[] output[0];
    delete[] input;
    delete[] output;
    CCombFilterIf::destroy(combFilterInterface);

    return return_value;
}

void runTests() {
    typedef bool (*fp)(); /* function pointer type */
    fp testFunctions[] = {
        test_1_FIR_cancel_out_when_frequency_matches,
        test_2_IIR_mag_change_when_frequency_matches,

    };

    for (int i = 0; i < 2; ++i) {
        fp testFunctionPointer = testFunctions[i];
        if (testFunctionPointer()) {
            std::printf("\033[32m[PASS]\033[39m");
        }
        else {
            std::printf("\033[31m[FAIL]\033[39m");
        }
        std::printf(" Test %d\n", i);
    }
}

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[]) {
    if (argc > 1) {
        /* First parse arguments, which is essential for audio file interface & combfilter initialization */
        try {
            CombFilterArgs_t args = parseArg(argc, argv);
            processFile(args);
        }
        catch (std::exception& e) {
            std::fprintf(stderr, "Error when trying to parse arguments: %s", e.what());
        }
    }
    else {
        std::printf("No argument given, run tests by default.\n");
        runTests();
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
