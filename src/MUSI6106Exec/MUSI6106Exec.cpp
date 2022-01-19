
#include <iostream>
#include <ctime>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"

using std::cout;
using std::endl;

// local function declarations
void    showClInfo ();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
    std::string             sInputFilePath,                 //!< file paths
                            sOutputFilePath;

    static const int        kBlockSize = 1024;

    clock_t                 time = 0;

    float                   **ppfAudioData = 0;

    CAudioFileIf            *phAudioFile = 0;
    std::fstream            hOutputFile;
    CAudioFileIf::FileSpec_t stFileSpec;

    showClInfo();

    //////////////////////////////////////////////////////////////////////////////
    // parse command line arguments
    if (argc == 3) {
        sInputFilePath = std::string(argv[1]);
        sOutputFilePath = std::string(argv[2]);
    }
    else {
        return 0;
    }
 
    //////////////////////////////////////////////////////////////////////////////
    // open the input wave file
    CAudioFileIf::create(phAudioFile);
    Error_t inputFileOpenStatus = phAudioFile->openFile(sInputFilePath, CAudioFileIf::FileIoType_t::kFileRead);
    if (inputFileOpenStatus != Error_t::kNoError) {
        throw inputFileOpenStatus;
    }
    phAudioFile->getFileSpec(stFileSpec);
    const int kNumChannel = stFileSpec.iNumChannels;
    const int kFs = stFileSpec.fSampleRateInHz;
 
    //////////////////////////////////////////////////////////////////////////////
    // open the output text file
    // std::ofstream hOutputFile(sOutputFilePath, std::ios::binary);
    hOutputFile = std::fstream(sOutputFilePath, std::ios::binary | std::ios::out);
 
    //////////////////////////////////////////////////////////////////////////////
    // allocate memory
    long long audioLengthInFrame = 0;
    phAudioFile->getLength(audioLengthInFrame);
    ppfAudioData = new float*[kNumChannel];
    for (int i = 0; i < kNumChannel; ++i) {
        ppfAudioData[i] = new float[kBlockSize];
    }
 
    //////////////////////////////////////////////////////////////////////////////
    // get audio data and write it to the output text file (one column per channel)
    long long kllBlockSize = static_cast<long long>(kBlockSize);
    for (long long pos = 0; pos < audioLengthInFrame; phAudioFile->getPosition(pos)) {
        phAudioFile->readData(ppfAudioData, kllBlockSize);
        for (long long i = 0; i < kllBlockSize; ++i) {
            for (int j = 0; j < kNumChannel; ++j) {
                if (j) hOutputFile << " ";
                hOutputFile << ppfAudioData[j][i];
            }
            hOutputFile << std::endl;
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    // clean-up (close files and free memory)
    hOutputFile.close();
    phAudioFile->closeFile();
    CAudioFileIf::destroy(phAudioFile);

    for (int i = 0; i < kNumChannel; ++i) {
        delete [] ppfAudioData[i];
    }
    delete [] ppfAudioData;

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

