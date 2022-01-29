
#include <iostream>
#include <ctime>

#include "MUSI6106Config.h"

#include "RingBuffer.h"

using std::cout;
using std::endl;

// local function declarations
void    showClInfo ();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
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

    // pressure test: test the container overflow behavior
    pCRingBuff->reset();
    for (int i = 0; i < 20; ++i) {
        pCRingBuff->putPostInc(1.F*i);
    }
    for (int i = 0; i < 17; ++i) {
        if (i) std::cout << ", ";
        std::cout << pCRingBuff->getPostInc();
    }
    std::cout << std::endl;

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
