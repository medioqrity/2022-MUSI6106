#if !defined(__RingBuffer_hdr__)
#define __RingBuffer_hdr__

#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstring>

/*! \brief implement a circular buffer of type T
*/
template <class T>
class CRingBuffer
{
public:
    explicit CRingBuffer(int iBufferLengthInSamples) :
        m_iBuffLength(iBufferLengthInSamples),
        m_iReadIdx(0),
        m_iWriteIdx(0),
        m_ptBuff(0)
    {
        assert(iBufferLengthInSamples > 0);

        m_ptBuff = new T[m_iBuffLength];
        reset();
    }

    virtual ~CRingBuffer()
    {
        delete[] m_ptBuff;
        m_ptBuff = 0;
    }

    /*! add a new value of type T to write index and increment write index
    \param tNewValue the new value
    \return void
    */
    void putPostInc(T tNewValue)
    {
        put(tNewValue);
        incIdx(m_iWriteIdx);
    }

    /*! add a new value of type T to write index
    \param tNewValue the new value
    \return void
    */
    void put(T tNewValue)
    {
        m_ptBuff[m_iWriteIdx] = tNewValue;
    }

    /*! return the value at the current read index and increment the read pointer
    \return float the value from the read index
    */
    T getPostInc()
    {
        T tValue = get();
        incIdx(m_iReadIdx);
        return tValue;
    }

    T get() const {
        return m_ptBuff[getReadIdx()];
    }

    T get(int iOffset) const {
        return m_ptBuff[indexWrapper(getReadIdx() + iOffset)];
    }

    /*! return the value at the current read index
    \param fOffset: read at offset from read index
    \return float the value from the read index
    */
    T get(float fOffset, bool print_lowerbound = false) const
    {
        int lowerBound = indexWrapper(static_cast<int>(fOffset));
        int upperBound = indexWrapper(lowerBound + 1);

        // determine if the upper bound should be enlarged.
        float fractionPart = abs(fOffset - static_cast<int>(fOffset));
        
        if (!(0 <= fractionPart && fractionPart <= 1)) {
            std::printf("%.8f %.8f\n", fOffset, fractionPart);
        }

        if (upperBound >= m_iBuffLength) {
            throw std::exception("The input offset is too big and exceeds the buffer length.");
        }

        // interpolate between lower & upper bound
        T fLowerBoundValue = get(lowerBound);
        T fUpperBoundValue = get(upperBound);

        return (1 - fractionPart) * fLowerBoundValue + fractionPart * fUpperBoundValue;
    }

    /*! set buffer content and indices to 0
    \return void
    */
    void reset()
    {
        std::memset (m_ptBuff, 0, sizeof(T)*m_iBuffLength);
        m_iReadIdx  = 0;
        m_iWriteIdx = 0;
    }

    /*! return the current index for writing/put
    \return int
    */
    int getWriteIdx() const
    {
        return m_iWriteIdx;
    }

    /*! move the write index to a new position
    \param iNewWriteIdx: new position
    \return void
    */
    void setWriteIdx(int iNewWriteIdx)
    {
        incIdx(m_iWriteIdx, iNewWriteIdx - m_iWriteIdx);
    }

    /*! return the current index for reading/get
    \return int
    */
    int getReadIdx() const
    {
        return m_iReadIdx;
    }

    /*! move the read index to a new position
    \param iNewReadIdx: new position
    \return void
    */
    void setReadIdx(int iNewReadIdx)
    {
        incIdx(m_iReadIdx, iNewReadIdx - m_iReadIdx);
    }

    /*! returns the number of values currently buffered (note: 0 could also mean the buffer is full!)
    \return int
    */
    int getNumValuesInBuffer() const
    {
        return (m_iWriteIdx - m_iReadIdx + m_iBuffLength) % m_iBuffLength;
    }

    /*! returns the length of the internal buffer
    \return int
    */
    int getLength() const
    {
        return m_iBuffLength;
    }

    int indexWrapper(int index) const {
        return (index < 0) ? m_iBuffLength - (-index % m_iBuffLength) : index % m_iBuffLength;
    }

private:
    CRingBuffer();
    CRingBuffer(const CRingBuffer& that);

    void incIdx(int& iIdx, int iOffset = 1)
    {
        while ((iIdx + iOffset) < 0)
        {
            // avoid negative buffer indices
            iOffset += m_iBuffLength;
        }
        iIdx = (iIdx + iOffset) % m_iBuffLength;
    };

    int m_iBuffLength = 0,      //!< length of the internal buffer
        m_iReadIdx = 0,         //!< current read index
        m_iWriteIdx = 0;        //!< current write index

    T* m_ptBuff = 0;            //!< data buffer
};
#endif // __RingBuffer_hdr__


// #if !defined(__RingBuffer_hdr__)
// #define __RingBuffer_hdr__
// 
// #include <cassert>
// #include <algorithm>
// #include <cmath>
// #include <cstring>
// 
// /*! \brief implement a circular buffer of type T
// */
// template <class T> 
// class CRingBuffer
// {
// public:
//     explicit CRingBuffer(int iBufferLengthInSamples) :
//         m_iBuffLength(iBufferLengthInSamples + 1)
//     {
//         assert(iBufferLengthInSamples > 0);
// 
//         // allocate and init
//         // use one empty space to make sure we can separate empty and full simply by
//         // checking indexies.
//         m_buffer = new T[iBufferLengthInSamples + 1];
//         for (int i = 0; i < m_iBuffLength; ++i) {
//             m_buffer[i] = 0;
//         }
// 
//         // the read and write index, and it represents range [m_iReadIdx, m_iWriteIdx)
//         // NOTE that it's possible to have (m_iReadIdx > m_iWriteIdx).
//         m_iReadIdx = 0;
//         m_iWriteIdx = 0;
//     }
// 
//     virtual ~CRingBuffer()
//     {
//         // free memory
//         delete[] m_buffer;
//     }
// 
//     /*! add a new value of type T to write index and increment write index
//     \param tNewValue the new value
//     \return void
//     */
//     void putPostInc (T tNewValue)
//     {
//         // put the value at the position, no matter if there exist a value
//         put(tNewValue);
//         incWriteIdx();
//         if (getReadIdx() == getWriteIdx()) incReadIdx();
//     }
// 
//     /*! add a new value of type T to write index
//     \param tNewValue the new value
//     \return void
//     */
//     void put(T tNewValue)
//     {
//         m_buffer[getWriteIdx()] = tNewValue;
//     }
//     
//     /*! return the value at the current read index and increment the read pointer
//     \return float the value from the read index
//     */
//     T getPostInc()
//     {
//         T value = get();
// 
//         // if the container is empty, then we need to also increase write index
//         if (getReadIdx() == getWriteIdx()) incWriteIdx();
// 
//         incReadIdx();
//         return value;
//     }
// 
//     /*! return the value at the current read index
//     \return float the value from the read index
//     */
//     T get() const
//     {
//         return m_buffer[getReadIdx()];
//     }
// 
//     T get(int iOffset) const {
//         return m_buffer[indexWrapper(getReadIdx() + iOffset)];
//     }
// 
//     /*! return the value at the current read index
//      * \param fOffset: read at offset from read index
//      * \return float the value from the read index*/
//     T get(float fOffset) const {
// 
//         int lowerBound = indexWrapper(static_cast<int>(fOffset) + getReadIdx());
//         int upperBound = indexWrapper(lowerBound + 1);
// 
//         // determine if the upper bound should be enlarged.
//         float integerPart = static_cast<int>(fOffset);
//         float fractionPart = fOffset - static_cast<float>(integerPart);
// 
//         if (upperBound >= m_iBuffLength) {
//             throw std::exception("The input offset is too big and exceeds the buffer length.");
//         }
// 
//         // interpolate between lower & upper bound
//         T fLowerBoundValue = get(indexWrapper(lowerBound));
//         T fUpperBoundValue = get(indexWrapper(upperBound));
// 
//         return (1 - fractionPart) * fLowerBoundValue + fractionPart * fUpperBoundValue;
//     }
//     
//     /*! set buffer content and indices to 0
//     \return void
//     */
//     void reset()
//     {
//         setReadIdx(0);
//         setWriteIdx(0);
//         for (int i = 0; i < m_iBuffLength; ++i) {
//             m_buffer[i] = 0;
//         }
//     }
// 
//     /*! return the current index for writing/put
//     \return int
//     */
//     int getWriteIdx() const
//     {
//         return m_iWriteIdx;
//     }
// 
//     /*! move the write index to a new position
//     \param iNewWriteIdx: new position
//     \return void
//     */
//     void setWriteIdx(int iNewWriteIdx)
//     {
//         m_iWriteIdx = indexWrapper(iNewWriteIdx);
//     }
// 
//     /*! return the current index for reading/get
//     \return int
//     */
//     int getReadIdx() const
//     {
//         return m_iReadIdx;
//     }
// 
//     /*! move the read index to a new position
//     \param iNewReadIdx: new position
//     \return void
//     */
//     void setReadIdx(int iNewReadIdx)
//     {
//         m_iReadIdx = indexWrapper(iNewReadIdx);
//     }
// 
//     /*! returns the number of values currently buffered (note: 0 could also mean the buffer is full!)
//     \return int
//     */
//     int getNumValuesInBuffer() const
//     {
//         int iNumValue = getWriteIdx() - getReadIdx();
//         while (iNumValue < 0) iNumValue += m_iBuffLength;
//         return iNumValue;
//     }
// 
//     /*! returns the length of the internal buffer
//     \return int
//     */
//     int getLength() const
//     {
//         return m_iBuffLength - 1;
//     }
// private:
//     CRingBuffer();
//     CRingBuffer(const CRingBuffer& that);
// 
//     int m_iBuffLength;              //!< length of the internal buffer
//     T* m_buffer;
//     int m_iReadIdx, m_iWriteIdx;
// 
//     void incReadIdx() {
//         inc(m_iReadIdx);
//     }
// 
//     void incWriteIdx() {
//         inc(m_iWriteIdx);
//     }
// 
//     void inc(int& i) {
//         ++i;
//         if (i >= m_iBuffLength) i -= m_iBuffLength; // should be faster than modulo
//     }
// 
//     inline int indexWrapper(int index) const {
//         return (index < 0) ? m_iBuffLength - (-index % m_iBuffLength) : index % m_iBuffLength;
//     }
// };
// #endif // __RingBuffer_hdr__
