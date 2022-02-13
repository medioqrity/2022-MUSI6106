
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

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
    std::string sInputFilePath,                 //!< file paths
                sOutputFilePath;

    static const int kBlockSize = 1024;

    clock_t time = 0;

    float **ppfAudioData = 0;

    CAudioFileIf *phAudioFile = 0;
    std::fstream hOutputFile;
    CAudioFileIf::FileSpec_t stFileSpec;
    CRingBuffer<float>* pCRingBuff = 0; 
    
    static const int kBlockSize = 17;

    showClInfo();

    pCRingBuff = new CRingBuffer<float>(kBlockSize);

    for (int i = 0; i < 5; i++)
    {
        pCRingBuff->putPostInc(1.F*i);
    }

    for (int i = 5; i < 30; i++)
    {
        std::cout << "i: " << i << ", pCRingBuff->getNumValuesInBuffer(): " << pCRingBuff->getNumValuesInBuffer();
        std::cout << ", pCRingBuff->getPostInc(): " << pCRingBuff->getPostInc() << std::endl;
        pCRingBuff->putPostInc(1.F*i);
    }

    // edge case test: test the container overflow behavior
    pCRingBuff->reset();
    for (int i = 0; i < 20; ++i) {
        pCRingBuff->putPostInc(1.F*i);
    }
    for (int i = 0; i < 17; ++i) {
        if (i) std::cout << ", ";
        std::cout << pCRingBuff->getPostInc();
    }
    std::cout << std::endl;

    // edge case test: test reading empty container behavior
    pCRingBuff->reset();
    pCRingBuff->putPostInc(1.F);
    for (int i = 0; i < 150; ++i) {
        if (i) std::cout << ", ";
        std::cout << pCRingBuff->getPostInc();
    }
    std::cout << std::endl;

    // edge case test: check if set read index as 0 and write index as n would make the container full.
    pCRingBuff->reset();
    pCRingBuff->setReadIdx(0);
    pCRingBuff->setWriteIdx(kBlockSize);
    for (int i = 0; i < 5; ++i) {
        pCRingBuff->putPostInc(2.F * (i + 1));
    }
    for (int i = 0; i < 17; ++i) {
        if (i) std::cout << ", ";
        std::cout << pCRingBuff->getPostInc();
    }
    std::cout << std::endl;

    // edge case test: check what would happen if we set index < 0 or index > kBlockSize
    pCRingBuff->reset();
    pCRingBuff->setReadIdx(-273);
    pCRingBuff->setWriteIdx(kBlockSize + 14);

    std::cout << "read index: " << pCRingBuff->getReadIdx() << ", write index: " << pCRingBuff->getWriteIdx() << std::endl;


    // assignment 1: Comb Filter
    
    
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
