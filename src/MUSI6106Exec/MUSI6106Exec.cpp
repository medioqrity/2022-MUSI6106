
#include <iostream>
#include <ctime>
#include <cassert>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "FastConv.h"
#include "Fft.h"

using std::cout;
using std::endl;

// local function declarations
void    showClInfo();

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
            // throw std::exception("failed to create audio file handler");
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
        // default:
        //     throw std::exception("wrong parameter of ioType (should be read / write)");
        }

        if (!phAudioFile->isOpen()) {
            // throw std::exception("failed to open file");
        }

        if (this->fileSpec == nullptr) {
            // throw std::exception("failed to load file spec");
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

    void readData(long long numSample) {
        if (ioType == CAudioFileIf::kFileRead) {
            phAudioFile->readData(buffer, numSample);
        }
    }

    void writeData(long long numSample) {
        if (ioType == CAudioFileIf::kFileWrite) {
            phAudioFile->writeData(buffer, numSample);
        }
    }

    bool isEof() const{
        // if (phAudioFile == nullptr) throw new std::exception("audio file handler has not been initialized");
        return phAudioFile->isEof();
    }

    int getNumChannels() const {
        // if (fileSpec == nullptr) throw new std::exception("file spec is nullptr, cannot get channel count");
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

    static const int            kBlockSize = 1024;
    long long                   iNumFrames = kBlockSize;

    float                       fModFrequencyInHz;
    float                       fModWidthInSec;

    clock_t                     time = 0;

    CFastConv* pCFastConv = new CFastConv();

    showClInfo();

    // command line args
    // if (argc != 4)
    // {
    //     cout << "Incorrect number of arguments!" << endl;
    //     return -1;
    // }
    sInputFilePath = "fake_id.wav";
    sIRFilePath = "IR.wav";
    sOutputFilePath = "out.wav";

    ///////////////////////////////////////////////////////////////////////////
    AudioFileWrapper inputAudio(sInputFilePath, kBlockSize, CAudioFileIf::kFileRead);
    AudioFileWrapper impulseResponse(sIRFilePath, -1, CAudioFileIf::kFileRead);
    AudioFileWrapper outputAudio(sOutputFilePath, kBlockSize, CAudioFileIf::kFileWrite, inputAudio.getFileSpec());

    ////////////////////////////////////////////////////////////////////////////
    impulseResponse.readData(impulseResponse.getNumSample());
    pCFastConv->init(impulseResponse.getBuffer()[0], impulseResponse.getNumSample(), kBlockSize, CFastConv::kFreqDomain);
    pCFastConv->setWetGain(0.1f);

    ////////////////////////////////////////////////////////////////////////////
    // processing
    while (!inputAudio.isEof())
    {
        printf("%lld\n", inputAudio.getHead());
        inputAudio.readData(iNumFrames);
        pCFastConv->process(outputAudio.getBuffer()[0], inputAudio.getBuffer()[0], iNumFrames);
        outputAudio.writeData(iNumFrames);
    }

    // flush remaining
    // float* remain = new float[impulseResponse.getNumSample()];
    // memset(remain, 0, sizeof(float) * impulseResponse.getNumSample());
    // pCFastConv->flushBuffer(remain);
    // and write remaining to output
    // it's ugly but currently no more elegant way to work around this
    // auto outputAudioFile = outputAudio.getAudioFile();
    // outputAudioFile->writeData(&remain, impulseResponse.getNumSample());

    cout << "\nreading/writing done in: \t" << (clock() - time) * 1.F / CLOCKS_PER_SEC << " seconds." << endl;

    //////////////////////////////////////////////////////////////////////////////
    // clean-up
    delete pCFastConv;
    // delete[] remain;

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

