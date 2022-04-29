#include <iostream>
#include <cassert>
#include <chrono>
#include <cstring>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "FastConv.h"
#include "Fft.h"

using std::cout;
using std::endl;

// local function declarations
void    showClInfo();

typedef struct ConvolverArgs {
    CFastConv::ConvCompMode_t convolver = CFastConv::ConvCompMode_t::kFreqDomain;
    float wetGain = 0.1f;
    int blockSize = 1 << 16;
    std::string inputPath = "fake_id.wav";
    std::string IRPath = "IR.wav";
    std::string outputPath = "out.wav";
} ConvolverArgs_t;

void printUsage() {
    std::printf("Usage: MUSI6106Exec.exe [option]\n\n");
    std::printf("Options and arguments:\n");
    std::printf("-t: The type of convolver to use. There are two choices available: time or freq.\n");
    std::printf("    -t time: use time domain filter. WARNING: SUPER SLOW!\n");
    std::printf("    -t freq: use frequency domain (fft-based) filter.\n");
    std::printf("-g: The gain parameter for the filter. \n");
    std::printf("    -g <gain>: set the wet gain of the convolved signal.\n");
    std::printf("-b: The internal FFT size.\n");
    std::printf("    -o <size>: set the internal FFT size when doing convolution. This option will have no\n");
    std::printf("               effect when the convolver is time domain filter.\n");
    std::printf("-i: The path to input wave file.\n");
    std::printf("    -i <path>: load the wave file of <path>. Notice you need to include filename.\n");
    std::printf("-r: The path to impulse response wave file.\n");
    std::printf("    -r <path>: load the impulse response at <path>. Notice you need to include filename.\n");
    std::printf("-o: The path to output wave file.\n");
    std::printf("    -o <path>: write the filtered signal into <path>. Notice you need to include filename\n");
    std::printf("-h --help: print this help message.\n");
    std::printf("Examples:\n");
    std::printf("- Read 'fake_id.wav' and convolve with `IR.wav` using FFT convolver, with wet gain 0.1,\n");
    std::printf("  FFT size 16384, write the filtered signal into 'out.wav':\n");
    std::printf("  `MUSI6106Exec.exe -t freq -g 0.1 -b 16384 -i fake_id.wav -r IR.wav -o out.wav`\n");
    std::printf("- Display this help:\n");
    std::printf("  `MUSI6106Exec.exe -h`\n");
    std::printf("  or\n");
    std::printf("  `MUSI6106Exec.exe --help`\n");
}

/*
Simple command line argument parser.
*/
ConvolverArgs_t parseArg(int argc, char** argv) {
    ConvolverArgs_t result;
    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') { /* this is an argument */
            if (argv[i][1] == 't') {
                if (i == argc - 1) throw(std::invalid_argument("Incomplete argument of comb filter type!"));
                ++i;
                if (!strcmp(argv[i], "time")) {
                    result.convolver = CFastConv::ConvCompMode_t::kTimeDomain;
                }
                else if (!strcmp(argv[i], "freq")) {
                    result.convolver = CFastConv::ConvCompMode_t::kFreqDomain;
                }
                else {
                    throw(std::invalid_argument("unknown argument!"));
                }
            }
            else if (argv[i][1] == 'g') {
                if (i == argc - 1) throw(std::invalid_argument("Incomplete argument of wet gain!"));
                result.wetGain = std::stof(argv[++i]);
            }
            else if (argv[i][1] == 'b') {
                if (i == argc - 1) throw(std::invalid_argument("Incomplete argument of block size!"));
                result.blockSize = std::stoi(argv[++i]);
            }
            else if (argv[i][1] == 'i') {
                if (i == argc - 1) throw(std::invalid_argument("Incomplete argument of input file path!"));
                result.inputPath = std::string(argv[++i]);
            }
            else if (argv[i][1] == 'r') {
                if (i == argc - 1) throw(std::invalid_argument("Incomplete argument of IR file path!"));
                result.IRPath = std::string(argv[++i]);
            }
            else if (argv[i][1] == 'o') {
                if (i == argc - 1) throw(std::invalid_argument("Incomplete argument of output file path!"));
                result.outputPath = std::string(argv[++i]);
            }
            else if (argv[i][1] == 'h' || !strcmp(argv[i], "--help")) {
                printUsage();
                exit(0);
            }
            else {
                throw(std::invalid_argument("unknown argument!"));
            }
        }
    }
    return result;
}


/*
    This class hides unimportant low-level details like:
        - Two phase initialization / deletion of `CAudioFileIf`
        - Nested initialization / deletion of audio buffer
        - Manage audio file related information (IO type, file spec)
        - Error handling

    This class also provides easier interface:
        - Only need `filepath`, `blockSize`, and `ioType` to create.

    This class represents the true "conceptual" audio file, which is easier to understand & use.
*/
class AudioFileWrapper {
public:
    AudioFileWrapper(
        const std::string& filepath, 
        int blockSize, 
        CAudioFileIf::FileIoType_t ioType, 
        CAudioFileIf::FileSpec_t* fileSpec = nullptr
    ): filepath(filepath), blockSize(blockSize), ioType(ioType), fileSpec(fileSpec) {
        CAudioFileIf::create(phAudioFile);

        if (phAudioFile == nullptr) {
            throw std::runtime_error("failed to create audio file handler");
        }

        switch (ioType)
        {
        case CAudioFileIf::kFileRead:
            phAudioFile->openFile(filepath, ioType);
            this->fileSpec = new CAudioFileIf::FileSpec_t();
            phAudioFile->getFileSpec(*(this->fileSpec));
            break;
        case CAudioFileIf::kFileWrite:
            phAudioFile->openFile(filepath, ioType, fileSpec);
            break;
        default:
            throw std::invalid_argument("wrong parameter of ioType (should be read / write)");
        }

        if (!phAudioFile->isOpen()) {
            throw std::runtime_error("failed to open file");
        }

        if (this->fileSpec == nullptr) {
            throw std::runtime_error("failed to load file spec");
        }

        phAudioFile->getLength(fileNumSample);

        // shortcut for loading the full audio file
        bool readFullFileFlag = this->blockSize < 0;
        if (readFullFileFlag) {
            this->blockSize = fileNumSample;
        }

        int numChannels = this->fileSpec->iNumChannels;
        buffer = new float* [numChannels];
        for (int i = 0; i < numChannels; ++i) {
            buffer[i] = new float[this->blockSize];
            memset(buffer[i], 0, sizeof(float) * this->blockSize);
        }
    }

    ~AudioFileWrapper() {
        int numChannels = fileSpec->iNumChannels;
        for (int i = 0; i < numChannels; ++i) {
            delete[] buffer[i];
            buffer[i] = nullptr;
        }
        delete[] buffer;
        buffer = nullptr;

        if (ioType == CAudioFileIf::kFileRead) {
            delete fileSpec;
            fileSpec = nullptr;
        }

        phAudioFile->closeFile();
        CAudioFileIf::destroy(phAudioFile);
    }

    CAudioFileIf::FileSpec_t* getFileSpec() {
        return fileSpec;
    }

    void readData(long long& numSample) {
        if (ioType == CAudioFileIf::kFileRead) {
            phAudioFile->readData(buffer, numSample);
        }
    }

    void writeData(long long numSample) {
        if (ioType == CAudioFileIf::kFileWrite) {
            phAudioFile->writeData(buffer, numSample);
        }
    }

    void writeData(float* outputBuffer, long long numSample) {
        if (ioType == CAudioFileIf::kFileWrite) {
            phAudioFile->writeData(&outputBuffer, numSample);
        }
    }

    bool isEof() const{
        if (phAudioFile == nullptr) throw new std::logic_error("audio file handler has not been initialized");
        return phAudioFile->isEof();
    }

    int getNumChannels() const {
        if (fileSpec == nullptr) throw new std::logic_error("file spec is nullptr, cannot get channel count");
        return fileSpec->iNumChannels;
    }

    long long getNumSample() const {
        return fileNumSample;
    }

    float getSampleRate() const {
        return fileSpec->fSampleRateInHz;
    }

    long long getHead() {
        long long head;
        phAudioFile->getPosition(head);
        return head;
    }

    float** getBuffer() {
        return buffer;
    }

    CAudioFileIf* getAudioFile() {
        return phAudioFile;
    }

private:
    std::string filepath;
    int blockSize;
    long long fileNumSample;
    float** buffer;
    CAudioFileIf::FileIoType_t ioType;
    CAudioFileIf::FileSpec_t* fileSpec = nullptr;
    CAudioFileIf* phAudioFile = nullptr;
};

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
    std::string             sInputFilePath,                 //!< file paths
                            sIRFilePath,
                            sOutputFilePath;

    CFastConv* pCFastConv = new CFastConv();

    // command line args
    ConvolverArgs_t args;
    try {
        args = parseArg(argc, argv);
    }
    catch (const std::exception& e) {
        printf("Error occurred during argument parsing: %s\n", e.what());
        printUsage();
        exit(0);
    }
    sInputFilePath = args.inputPath;
    sIRFilePath = args.IRPath;
    sOutputFilePath = args.outputPath;

    ///////////////////////////////////////////////////////////////////////////
    AudioFileWrapper inputAudio(sInputFilePath, args.blockSize, CAudioFileIf::kFileRead);
    AudioFileWrapper impulseResponse(sIRFilePath, -1, CAudioFileIf::kFileRead);
    AudioFileWrapper outputAudio(sOutputFilePath, args.blockSize, CAudioFileIf::kFileWrite, inputAudio.getFileSpec());

    ////////////////////////////////////////////////////////////////////////////
    long long irNumSample = impulseResponse.getNumSample();
    impulseResponse.readData(irNumSample);
    pCFastConv->init(impulseResponse.getBuffer()[0], impulseResponse.getNumSample(), args.blockSize, args.convolver);
    pCFastConv->setWetGain(args.wetGain);

    long long iNumFrames = args.blockSize;
    long long processDuration = 0;

    ////////////////////////////////////////////////////////////////////////////
    // processing
    while (!inputAudio.isEof())
    {
        inputAudio.readData(iNumFrames);

        auto start = std::chrono::steady_clock::now();
        pCFastConv->process(outputAudio.getBuffer()[0], inputAudio.getBuffer()[0], iNumFrames);
        processDuration += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();

        outputAudio.writeData(iNumFrames);
    }

    // flush remaining
    float* remain = new float[impulseResponse.getNumSample() - 1 + args.blockSize]();
    pCFastConv->flushBuffer(remain);
    outputAudio.writeData(remain, impulseResponse.getNumSample() - 1 + args.blockSize);

    cout << "Accumulated processing time: \t" << processDuration << " ns." << endl;

    //////////////////////////////////////////////////////////////////////////////
    // clean-up
    delete pCFastConv;
    delete[] remain;

    // all done
    return 0;
}


void     showClInfo()
{
    cout << "MUSI6106 Assignment Executable" << endl;
    cout << "(c) 2014-2022 by Alexander Lerch" << endl;
    cout << endl;

    return;
}

