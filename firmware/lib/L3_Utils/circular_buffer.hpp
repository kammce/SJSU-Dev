/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */

/**
* @file
* @brief Circular buffer
* @ingroup Utilities
*
* Version: 20140305    Initial
*/
#ifndef CIRCULAR_BUFFER_HPP__
#define CIRCULAR_BUFFER_HPP__

#include <stdlib.h>
#include <stdint.h>
#include <iterator>



/**
 * Circular buffer class
 * @ingroup Utilities
 *
 * Usage:
 * @code
    CircularBuffer <int> b(3);
    b.push_back(1);
    b.push_back(2);
    b.push_back(3);
    b.push_back(0);       // Will fail since buffer is full
    b.push_back(4, true); // Overwrite oldest data

    // Read the elements without popping the data, access them using index operator
    // Should be "2 3 4" since we previously over-wrote "1" with "4"
    printf("\nContents using index operator: ");
    for (uint32_t i = 0; i < b.size(); i++) {
        printf("%i ", b[i]);
    }

    // Use the iterator to read data without popping the data off the buffer
    // Should be "2 3 4"; same as using the index operator.
    printf("\nContents using iterator: ");
    for(CircularBuffer<int>::iterator cb = b.begin(); cb != b.end(); ++cb)
    {
        printf("%i ", *(cb));
    }

    // Pop the data from the buffer, should pop "2 3 4"
    int i = 0;
    while (b.pop_front(&i)) {
        printf("\nPopped %i", i);
    }
 * @endcode
 */
template <typename TYPE>
class CircularBuffer
{
public:
    CircularBuffer(uint32_t capacity);                      ///< Constructor with initial capacity as buffer size
    CircularBuffer(const CircularBuffer& copy);             ///< Copy Constructor
    CircularBuffer& operator=(const CircularBuffer& copy);  ///< = Operator to copy the buffer
    ~CircularBuffer();                                      ///< Destructor of the buffer

    /**
     * Write to end of buffer.
     * @param data        The data to write.
     * @param forceWrite  Optional parameter, if true, and the buffer is full,
     *                    will force a write, discarding oldest data.
     * @return            true if successful or false if no capacity
     */
    bool push_back(const TYPE data, bool forceWrite = false);

    TYPE pop_front(void);            ///< @returns the oldest element
    bool pop_front(TYPE* dataPtr);   ///< @returns true if an element is available to be read, and is read to dataPtr
    TYPE peek_front(void);           ///< @returns the oldest element, but doesn't remove it from the buffer
    bool peek_front(TYPE* dataPtr);  ///< @returns true if an element is available, and is read into dataPtr

    /// @returns the number of elements in the array
    uint32_t size(void) const     { return mCount;    }

    /// @returns the capacity of the circular buffer
    uint32_t capacity(void) const { return mCapacity; }

    /// Clears the contents of the buffer
    void clear(void) { mCount = mWriteIndex = mReadIndex = 0; }

    /// += Operator which is same as push_back() of an item
    void operator+=(TYPE item) { push_back(item); }

    /**
     * Index operator.  This will always return in the FIFO order, so
     * index 0 represents the OLDEST data.  The max index value should
     * not go beyond [getCapacity() - 1]
     */
    TYPE& operator [] (uint32_t index) const { return mpArray[ (index + mReadIndex) % mCapacity]; }


    /****************************************************************/
    /**** Iterators ****/
    /****************************************************************/
    typedef int size_type;

    class iterator
    {
        public:
            typedef iterator self_type;
            typedef TYPE value_type;
            typedef TYPE& reference;
            typedef TYPE* pointer;
            typedef std::forward_iterator_tag iterator_category;
            typedef int difference_type;

            iterator(CircularBuffer<TYPE> *p) : mpCb(p), mIndex(0) { }

            /// Preincrement operator ie: ++iterator
            self_type operator++()           { self_type i = *this; mIndex++; return i; }

            /// Postincrement operator ie: iterator++
            self_type operator++(int unused) { mIndex++; self_type i = *this; return i; }

            /// * operator to get the value ie: *iterator OR iterator.operator *()
            reference operator*() { return mpCb->operator [] (mIndex); }

            /// pointer operator to get the address ie: *(iterator.operator->())
            pointer operator->()  { return &( mpCb->operator [] (mIndex)); }

            /// != operator between iterators
            bool operator!=(const self_type& rhs) { return (mpCb != rhs.mpCb) ? false : (mIndex < mpCb->size());  }

            /// == operator between iterators
            bool operator==(const self_type& rhs) { return (mpCb != rhs.mpCb) ? false : (mIndex >= mpCb->size()); }

        private:
            CircularBuffer<TYPE> *mpCb; ///< The circular buffer pointer
            uint32_t mIndex;            ///< Index, starting with the oldest data
    };

    class const_iterator
    {
        public:
            typedef const_iterator self_type;
            typedef TYPE value_type;
            typedef TYPE& reference;
            typedef TYPE* pointer;
            typedef std::forward_iterator_tag iterator_category;
            typedef int difference_type;

            const_iterator(CircularBuffer<TYPE> *p) : mpCb(p), mIndex(0) { }

            /// Preincrement operator ie: ++iterator
            self_type operator++()           { self_type i = *this; mIndex++; return i; }

            /// Postincrement operator ie: iterator++
            self_type operator++(int unused) { mIndex++; self_type i = *this; return i; }

            /// * operator to get the value ie: *iterator OR iterator.operator *()
            reference operator*() { return mpCb->operator [] (mIndex); }

            /// pointer operator to get the address ie: *(iterator.operator->())
            pointer operator->()  { return &( mpCb->operator [] (mIndex)); }

            /// != operator between iterators
            bool operator!=(const self_type& rhs) { return (mpCb != rhs.mpCb) ? false : (mIndex < mpCb->size());  }

            /// == operator between iterators
            bool operator==(const self_type& rhs) { return (mpCb != rhs.mpCb) ? false : (mIndex >= mpCb->size()); }

        private:
            CircularBuffer<TYPE> *mpCb; ///< The circular buffer pointer
            uint32_t mIndex;            ///< Index, starting with the oldest data
    };



    /**
     * @{ Get iterators
     */
    iterator begin() { return iterator(this); }
    iterator end()   { return iterator(this); }
    const_iterator begin() const { const_iterator(this); }
    const_iterator end()  const { const_iterator(this);  }
    /** @} */



private:
    /// Private constructor; do not use this constructor.
    CircularBuffer() : mCapacity(0), mWriteIndex(0), mReadIndex(0), mCount(0), mpArray(0) { }

    const uint32_t mCapacity;   ///< The capacity of the circular buffer
    uint32_t mWriteIndex;       ///< Next write index
    uint32_t mReadIndex;        ///< Next read index
    uint32_t mCount;            ///< The count of the number of elements
    TYPE *mpArray;              ///< The array of elements
    friend class iterator;      ///< Iterator is our friend...

    /**
     * Initializes all member variables of this vector.
     * This is used such that constructors can call a single init function
     * rather than each having to initialize the variables separately.
     */
    void init(void)
    {
        mWriteIndex = 0;
        mReadIndex  = 0;
        mCount  = 0;
        mpArray = new TYPE[mCapacity];
    }
};
















template <typename TYPE>
CircularBuffer<TYPE>::CircularBuffer(uint32_t cap) : mCapacity(cap)
{
    init();
}
template <typename TYPE>
CircularBuffer<TYPE>::CircularBuffer(const CircularBuffer& copy) : mCapacity(copy.capacity())
{
    *this = copy; // Call = Operator below to copy vector contents
}

template <typename TYPE>
CircularBuffer<TYPE>& CircularBuffer<TYPE>::operator=(const CircularBuffer<TYPE>& copy)
{
    if(this != &copy)
    {
        this->init(); // capacity is initialized by constructor

        // Now copy other contents into this object
        for(unsigned int i = 0; i < copy.size(); i++)
        {
            this->push_back(copy[i]);
        }
    }
    return *this;
}
template <typename TYPE>
CircularBuffer<TYPE>::~CircularBuffer()
{
    delete [] mpArray;
}

template <typename TYPE>
bool CircularBuffer<TYPE>::push_back(const TYPE data, bool forceWrite)
{
    bool success = false;

    if (mCount < mCapacity || forceWrite)
    {
        /**
         * If we don't have the capacity, then perform a read()
         * to pop the oldest element, and then cap the count.
         */
        if (++mCount > mCapacity) {
            (void) pop_front();
        }
        success = true;

        mpArray[mWriteIndex] = data;
        if (++mWriteIndex >= mCapacity) {
            mWriteIndex = 0;
        }
    }

    return success;
}

template <typename TYPE>
TYPE CircularBuffer<TYPE>::pop_front(void)
{
    TYPE data = 0;

    if (mCount > 0) {
        --mCount;
        data = mpArray[mReadIndex];
        if (++mReadIndex >= mCapacity) {
            mReadIndex = 0;
        }
    }

    return data;
}

template <typename TYPE>
bool CircularBuffer<TYPE>::pop_front(TYPE* dataPtr)
{
    bool success = (mCount > 0);
    *dataPtr = pop_front();
    return success;
}

template <typename TYPE>
TYPE CircularBuffer<TYPE>::peek_front(void)
{
    TYPE data = 0;
    if (mCount > 0) {
        data = mpArray[mReadIndex];
    }
    return data;
}

template <typename TYPE>
bool CircularBuffer<TYPE>::peek_front(TYPE* dataPtr)
{
    bool success = (mCount > 0);
    *dataPtr = peek_front();
    return success;
}



#ifdef TESTING
#include <assert.h>
static inline void test_CircularBuffer(void)
{
    CircularBuffer <int> b(3);

    assert(3 == b.capacity());
    assert(0 == b.size());

    assert(b.push_back(1));
    assert(b.push_back(2));
    assert(b.push_back(3));
    assert(3 == b.capacity());
    assert(3 == b.size());
    assert(1 == b[0]);
    assert(2 == b[1]);
    assert(3 == b[2]);
    assert(!b.push_back(4));
    assert(b.push_back(4, true));
    assert(3 == b.capacity());
    assert(3 == b.size());

    assert(2 == b[0]);
    assert(3 == b[1]);
    assert(4 == b[2]);
    assert(2 == b.pop_front());
    assert(3 == b.pop_front());
    assert(4 == b.pop_front());

    int x;
    assert(!b.pop_front(&x));

    b.clear();
    b.push_back(1);
    b.push_back(2);
    b.push_back(3);

    /**
     * At this point:
     * write index = 1
     * read  index = 1
     * | 1 | 2 | 3 |
     * | 4 | 2 | 3 |
     */
    do {
        CircularBuffer<int>::iterator cb = b.begin();
        assert(*cb == 1); cb++;
        assert(*cb == 2); cb++;
        assert(*cb == 3); cb++;
    } while(0);

    do {
        CircularBuffer<int>::iterator cb = b.begin();
        assert(*(cb.operator->()) == 1); ++cb;
        assert(*(cb.operator->()) == 2); ++cb;
        assert(*(cb.operator->()) == 3); ++cb;
    } while(0);

    b.clear();
    b.push_back(1);
    b.push_back(2);
    b.push_back(3);

    // Copy constructor test
    CircularBuffer <int> b2 = b;
    assert(1 == b2[0]);
    assert(2 == b2[1]);
    assert(3 == b2[2]);
    b2.push_back(4, true); // Overwrite oldest data
    assert(3 == b2.size());
    assert(2 == b2.pop_front());
    assert(3 == b2.pop_front());
    assert(4 == b2.pop_front());

    // Write the elements without popping the data, access them using index operator
    assert( b2.push_back(1));
    assert( b2.push_back(2));
    assert(1 == b2[0]);
    assert(2 == b2[1]);

    CircularBuffer<int>::iterator cb = b2.begin();
    assert(2 == b2.size());
    assert(3 == b2.capacity());
    assert(1 == *cb);   cb++;
    assert(2 == *cb);   cb++;
    assert(1 != *cb);   cb++;

    assert( b2.push_back(3));
    cb = b2.begin();
    assert(1 == *cb);   cb++;
    assert(2 == *cb);   cb++;
    assert(3 == *cb);   cb++;

    assert( !b2.push_back(4));
    cb = b2.begin();
    assert(1 == *cb);   cb++;
    assert(2 == *cb);   cb++;
    assert(3 == *cb);   cb++;

    assert( b2.push_back(4, true));
    cb = b2.begin();
    assert(3 == b2.size());
    assert(3 == b2.capacity());
    assert(2 == *cb);   cb++;
    assert(3 == *cb);   cb++;
    assert(4 == *cb);   cb++;

    do {
        CircularBuffer <int> b(3);
        b.push_back(1);
        b.push_back(2);
        b.push_back(3);
        b.push_back(4, true); // Overwrite oldest data
        assert(3 == b.capacity());
        assert(3 == b.size());

        // Write the elements without popping the data, access them using index operator
        printf("\nContents using index operator: ");
        // Should be "2 3 4"
        for (uint32_t i = 0; i < b.size(); i++) {
            printf("%i ", b[i]);
        }

        // Use the iterator to read data without popping the data off the buffer
        // Should be "2 3 4"
        printf("\nContents using iterator: ");
        for(CircularBuffer<int>::iterator cb = b.begin(); cb != b.end(); ++cb)
        {
            printf("%i ", *(cb));
        }

        int i = 0;
        while ((i = b.pop_front())) {
            printf("\nPopped %i", i);
        }
    } while(0);

    puts("\nCircular Buffer Tests Successful!");
}
#endif /* #ifdef TESTING */



#endif /* #ifndef CIRCULAR_BUFFER_HPP__ */
