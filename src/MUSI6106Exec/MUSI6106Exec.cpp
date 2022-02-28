
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

void printUsage() {
    std::printf("Usage: MUSI6106Exec.exe [option]\n\n");
    std::printf("Options and arguments:\n");
    std::printf("-t: The type of comb filter to use. There are two choices available: FIR or IIR.\n");
    std::printf("    -t FIR: use FIR comb filter.\n");
    std::printf("    -t IIR: use IIR comb filter.\n");
    std::printf("-d: The length of delay in second.\n");
    std::printf("    -d <delay>: set the delay time in second of the comb filter. For good comb effect this\n");
    std::printf("                value should be around 0.001-0.01. The MAXIMUM possible delay length is 1s\n");
    std::printf("-g: The gain parameter for the filter. \n");
    std::printf("    -g <gain>: set the gain of the comb filter, which is similar to the amount of 'feedback'.\n");
    std::printf("               The higher this value the 'sharper' the result is.\n");
    std::printf("-i: The path to input wave file.\n");
    std::printf("    -i <path>: load the wave file of <path>. Notice you need to include filename.\n");
    std::printf("-o: The path to output wave file.\n");
    std::printf("    -o <path>: write the filtered signal into <path>. Notice you need to include filename\n");
    std::printf("-h --help: print this help message.\n");
    std::printf("Examples:\n");
    std::printf("- Read 'fake_id.wav' and filter it using a FIR comb filter, with delay time 0.001s and gain 0.9,\n");
    std::printf("  write the filtered signal into 'out.wav':\n");
    std::printf("  `MUSI6106Exec.exe -t FIR -d 0.001 -g 0.9 -i fake_id.wav -o out.wav`\n");
    std::printf("- Run test:\n");
    std::printf("  `MUSI6106Exec.exe`\n");
    std::printf("- Display this help:\n");
    std::printf("  `MUSI6106Exec.exe -h`\n");
    std::printf("  or\n");
    std::printf("  `MUSI6106Exec.exe --help`\n");
}

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
            else if (argv[i][1] == 'h' || !strcmp(argv[i], "--help")) {
                printUsage();
                exit(0);
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


void runTests() {
    typedef bool (*fp)(); /* function pointer type */
    fp testFunctions[] = {nullptr};

    for (int i = 0; i < 0; ++i) {
        fp testFunctionPointer = testFunctions[i];
        if (testFunctionPointer()) {
            std::printf("\033[32m[PASS]\033[39m");
        }
        else {
            std::printf("\033[31m[FAIL]\033[39m");
        }
        std::printf(" Test %d\n", i + 1);
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
        printUsage();
        std::printf("[WARN] No argument given, run tests by default.\n");
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
