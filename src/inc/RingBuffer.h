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
        m_iBuffLength(iBufferLengthInSamples + 1)
    {
        assert(iBufferLengthInSamples > 0);

        // allocate and init
        // use one empty space to make sure we can separate empty and full simply by
        // checking indexies.
        m_buffer = new T[iBufferLengthInSamples + 1];
        for (int i = 0; i < m_iBuffLength; ++i) {
            m_buffer[i] = 0;
        }

        // the read and write index, and it represents range [m_iReadIdx, m_iWriteIdx)
        // NOTE that it's possible to have (m_iReadIdx > m_iWriteIdx).
        m_iReadIdx = 0;
        m_iWriteIdx = 0;
    }

    virtual ~CRingBuffer()
    {
        // free memory
        delete[] m_buffer;
    }

    /*! add a new value of type T to write index and increment write index
    \param tNewValue the new value
    \return void
    */
    void putPostInc (T tNewValue)
    {
        // put the value at the position, no matter if there exist a value
        put(tNewValue);
        incWriteIdx();
        if (getReadIdx() == getWriteIdx()) incReadIdx();
    }

    /*! add a new value of type T to write index
    \param tNewValue the new value
    \return void
    */
    void put(T tNewValue)
    {
        m_buffer[getWriteIdx()] = tNewValue;
    }
    
    /*! return the value at the current read index and increment the read pointer
    \return float the value from the read index
    */
    T getPostInc()
    {
        T value = get();
        incReadIdx();
        return value;
    }

    /*! return the value at the current read index
    \return float the value from the read index
    */
    T get() const
    {
        return m_buffer[getReadIdx()];
    }
    
    /*! set buffer content and indices to 0
    \return void
    */
    void reset()
    {
        setReadIdx(0);
        setWriteIdx(0);
        for (int i = 0; i < m_iBuffLength; ++i) {
            m_buffer[i] = 0;
        }
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
        m_iWriteIdx = iNewWriteIdx % m_iBuffLength;
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
        m_iReadIdx = iNewReadIdx % m_iBuffLength;
    }

    /*! returns the number of values currently buffered (note: 0 could also mean the buffer is full!)
    \return int
    */
    int getNumValuesInBuffer() const
    {
        int iNumValue = getWriteIdx() - getReadIdx();
        while (iNumValue < 0) iNumValue += m_iBuffLength;
        return iNumValue;
    }

    /*! returns the length of the internal buffer
    \return int
    */
    int getLength() const
    {
        return m_iBuffLength - 1;
    }
private:
    CRingBuffer();
    CRingBuffer(const CRingBuffer& that);

    int m_iBuffLength;              //!< length of the internal buffer
    T* m_buffer;
    int m_iReadIdx, m_iWriteIdx;

    void incReadIdx() {
        (++m_iReadIdx) %= m_iBuffLength;
    }

    void incWriteIdx() {
        (++m_iWriteIdx) %= m_iBuffLength;
    }
};
#endif // __RingBuffer_hdr__
