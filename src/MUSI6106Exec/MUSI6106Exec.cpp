
#include <iostream>
#include <ctime>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "CombFilterIf.h"
#include "RingBuffer.h"
#include "Vibrato.h"

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

typedef struct VibratoArgs {
    CCombFilterIf::CombFilterType_t filterType = CCombFilterIf::CombFilterType_t::kCombFIR;
    float vibratoWidth = 0.01F;
    float vibratoFrequency = 6.6F;
    std::string inputPath = "sin.wav";
    std::string outputPath = "out.wav";
} VibratoArgs_t;

void printUsage() {
    std::printf("Usage: MUSI6106Exec.exe [option]\n\n");
    std::printf("Options and arguments:\n");
    std::printf("-w: The vibrato width in Hz.\n");
    std::printf("    -w <width>: set the width of vibrato effector in Hz. E.g. for 440 sinusoid & 10 Hz vibrato\n");
    std::printf("                width, it will oscillate in 430-450Hz.\n");
    std::printf("-f: The vibrato frequency in Hz.\n");
    std::printf("    -f <freq>: set the frequency of vibrato effector in Hz. E.g. Frequency 6Hz -> vibrato 6 times/s.\n");
    std::printf("-i: The path to input wave file.\n");
    std::printf("    -i <path>: load the wave file of <path>. Notice you need to include filename.\n");
    std::printf("-o: The path to output wave file.\n");
    std::printf("    -o <path>: write the filtered signal into <path>. Notice you need to include filename\n");
    std::printf("-h --help: print this help message.\n");
    std::printf("Examples:\n");
    std::printf("- Read 'fake_id.wav' and apply vibrato effector, with vibrato width=10Hz, frequency=6.6Hz,\n");
    std::printf("  write the filtered signal into 'out.wav':\n");
    std::printf("  `MUSI6106Exec.exe -w 10 -f 6.6 -i fake_id.wav -o out.wav`\n");
    std::printf("- Display this help:\n");
    std::printf("  `MUSI6106Exec.exe -h`\n");
    std::printf("  or\n");
    std::printf("  `MUSI6106Exec.exe --help`\n");
}

/*
Simple command line argument parser.
*/
VibratoArgs_t parseArg(int argc, char** argv) {
    VibratoArgs_t result;
    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') { /* this is an argument */
            if (argv[i][1] == 'w') {
                if (i == argc - 1) throw(std::exception("Incomplete argument of delay!"));
                result.vibratoWidth = std::stof(argv[++i]);
            }
            else if (argv[i][1] == 'f') {
                if (i == argc - 1) throw(std::exception("Incomplete argument of gain!"));
                result.vibratoFrequency = std::stof(argv[++i]);
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

void processFile(VibratoArgs_t& args) {
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

    /* Initialize Vibrato Effector */
    VibratoEffector* vibratoEffector = new VibratoEffector(static_cast<int>(stInputFileSpec.fSampleRateInHz), stInputFileSpec.iNumChannels, args.vibratoFrequency, args.vibratoWidth);

    /* Initialize audio buffer arrays */
    long long kllBlockSize = static_cast<long long>(kBlockSize);

    float** ppfInputAudioData, **ppfOutputAudioData;
    ppfInputAudioData = new float* [stInputFileSpec.iNumChannels];
    ppfOutputAudioData = new float* [stInputFileSpec.iNumChannels];
    for (int i = 0; i < stInputFileSpec.iNumChannels; ++i) {
        ppfInputAudioData[i] = new float[kllBlockSize];
        ppfOutputAudioData[i] = new float[kllBlockSize];
    }

    /* set lfo shape */
    float* temp = new float[1 << 12];
    CSynthesis::generateSine(temp, 1, 1 << 12, 1 << 12);
    vibratoEffector->setLfoShape(temp);
    delete[] temp;

    /* Iterate over whole file and apply effect */
    for (; !phInputAudioFile->isEof(); ) {
        phInputAudioFile->readData(ppfInputAudioData, kllBlockSize);
        vibratoEffector->process(ppfInputAudioData, ppfOutputAudioData, kllBlockSize);
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

    delete vibratoEffector;
    CAudioFileIf::destroy(phInputAudioFile);
    CAudioFileIf::destroy(phOutputAudioFile);
}

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[]) {
    if (argc > 1) {
        /* First parse arguments, which is essential for audio file interface & combfilter initialization */
        try {
            VibratoArgs_t args = parseArg(argc, argv);
            processFile(args);
        }
        catch (std::exception& e) {
            std::fprintf(stderr, "Error when trying to parse arguments: %s", e.what());
        }
    }
    else {
        printUsage();
        std::printf("[WARN] No argument given.\n");
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
