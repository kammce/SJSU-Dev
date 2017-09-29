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
* @file vector.hpp
* @brief  Vector Class with a small footprint
* @ingroup Utilities
*
* Version: 05172013    Added at() to ease element access when vector is a pointer.
* Version: 06192012    Initial
*/
#ifndef _VECTOR_H__
#define _VECTOR_H__

#include <stdlib.h>



/**
 * Vector class
 * @ingroup Utilities
 *
 * This vector class can by used as a dynamic array.
 * This can provide fast-index based retrieval of stored elements
 * and also provides fast methods to erase or rotate the elements.
 * The underlining storage uses pointer-based storage and entire
 * objects are not created or deleted when the vector contents change.
 *
 * Usage:
 * @code
 *  VECTOR<int> intVec;
 *  intVec += 1;
 *  intVec += 2;
 *  intVec += 3;
 *
 *  intVec.remove(2);    // Vector now: 1 3
 *  intVec.rotateLeft(); // 1 3 --> 3 1
 *  printf("%i %i", intVec[0], intVec[1]); // Prints: 3 1
 * @endcode
 */
template <typename TYPE>
class VECTOR
{
public:
    VECTOR();                               ///< Default Constructor
    VECTOR(int initialCapacity);            ///< Constructor with initial capacity as Vector size
    VECTOR(const VECTOR& copy);             ///< Copy Constructor
    VECTOR& operator=(const VECTOR& copy);  ///<  =Operator to copy the vector.
    ~VECTOR();                              ///< Destructor of the vector

    const TYPE& front();                    ///< @returns the first(oldest) element of the vector (index 0).
    const TYPE& back();                     ///< @returns the last added element of the vector.
    const TYPE& pop_front();                ///< Pops & returns the first(oldest) element of the vector (index 0).  (SLOW)
    const TYPE& pop_back();                 ///< Pops & returns the last element from the vector. (FAST)
    void push_back(const TYPE& element);    ///< Pushes the element to the end of the vector. (FAST)
    void push_front(const TYPE& element);   ///< Pushes the element at the 1st location (index 0).  (SLOW)

    void reverse();             ///< Reverses the order of the vector contents.
    const TYPE& rotateRight();  ///< Rotates the vector right by 1 and @returns front() value
    const TYPE& rotateLeft();   ///< Rotates the vector left by 1 and @returns  front() value

    const TYPE& eraseAt(unsigned int pos);  ///< Erases the element at pos and returns it. All elements are shifted left from this pos.
    int  getFirstIndexOf(const TYPE& find); ///< @returns the first index at which the element find is located at
    bool remove(const TYPE& element);       ///< Removes the first Vector Element match from this vector, @returns true if successful
    int  removeAll(const TYPE& element);    ///< Removes all Vector Elements that match the given element, @returns number of elements removed

    bool replace(const TYPE& find, const TYPE& replace);    ///< Replaces the first element "find" and replaces it with "replace"
    int  replaceAll(const TYPE& find, const TYPE& replace); ///< Replaces all instances of "find" and replaces it with "replace"

    void fill(const TYPE& fillElement);       ///< Fills the entire vector capacity with the given fillElement.
    void fillUnused(const TYPE& fillElement); ///< Fills the unused capacity of the vector with the given fillElement.

    unsigned int size() const;          ///< @returns The size of the vector (actual usage)
    unsigned int capacity() const;      ///< @returns The capacity of the vector (allocated memory)
    void reserve(unsigned int size);    ///< Reserves the memory for the vector up front.
    void setGrowthFactor(int factor);   ///< Changes the size the vector grows by.
    void clear();                       ///< Clears the entire vector
    bool isEmpty();                     ///< @returns True if the vector is empty

    TYPE& at(const unsigned int i);                         ///< Access element at given index
    TYPE& operator[](const unsigned int i );                ///< [] Operator for Left-hand-side.
    const TYPE& operator[](const unsigned int i ) const;    ///< [] Operator of Right-hand-side.
    void operator+=(const TYPE& item) { push_back(item); }  ///< += Operator which is same as push_back() of an item

private:
    void changeCapacity(unsigned int newSize);      ///< Changes the capacity of this vector to the new size and handles internal memory move
    TYPE* shiftLeftFromPosition(unsigned int pos);  ///< @returns Pointer at the pos, and all pointers get shifted left from this pos
    TYPE* shiftRightFromPosition(unsigned int pos); ///< @returns Pointer at the last position, and all pointers get shifted right from ths pos

    unsigned int mGrowthRate;       ///< Number of elements added when vector needs to grow
    unsigned int mVectorCapacity;   ///< Capacity of this vector
    unsigned int mVectorSize;       ///< Used size of this vector
    TYPE **mpObjPtrs;               ///< Pointer array of TYPE
    TYPE mNullItem;                 ///< Null Item is returned when invalid vector element is accessed

    /// Initializes all member variables of this vector
    void init()
    {
        mGrowthRate = 4;
        mVectorCapacity = 0;
        mVectorSize = 0;
        mpObjPtrs   = 0;
    }
};
















template <typename TYPE>
VECTOR<TYPE>::VECTOR()
{
    init();
}
template <typename TYPE>
VECTOR<TYPE>::VECTOR(int initialCapacity)
{
    init();
    changeCapacity(initialCapacity);
}

template <typename TYPE>
VECTOR<TYPE>::VECTOR(const VECTOR& copy)
{
    init();
    *this = copy; // Call = Operator below to copy vector contents
}

template <typename TYPE>
VECTOR<TYPE>& VECTOR<TYPE>::operator=(const VECTOR<TYPE>& copy)
{
    if(this != &copy)
    {
        // Clear this vector and reserve enough for the vector to copy
        this->clear();
        this->reserve(copy.capacity());

        // Now copy other vectors contents into this vector
        for(unsigned int i = 0; i < copy.size(); i++)
        {
            *(this->mpObjPtrs[i]) = copy[i];
        }
    }
    return *this;
}

template <typename TYPE>
VECTOR<TYPE>::~VECTOR()
{
    for(unsigned int i=0; i < mVectorCapacity; i++) {
        delete mpObjPtrs[i];
    }
    delete [] mpObjPtrs;
}


template <typename TYPE>
const TYPE& VECTOR<TYPE>::pop_back()
{
    return (mVectorSize > 0) ? *mpObjPtrs[--mVectorSize] : mNullItem;
}


template <typename TYPE>
const TYPE& VECTOR<TYPE>::pop_front()
{
    return eraseAt(0);
}

template <typename TYPE>
void VECTOR<TYPE>::push_back(const TYPE& element)
{
    if(mVectorSize >= mVectorCapacity)
    {
        changeCapacity( (mVectorCapacity + mGrowthRate) );
    }
    *mpObjPtrs[mVectorSize++] = element;
}

template <typename TYPE>
void VECTOR<TYPE>::push_front(const TYPE& element)
{
    if(mVectorSize >= mVectorCapacity)
    {
        changeCapacity( (mVectorCapacity + mGrowthRate) );
    }

    // Make room to put new item at mpData[0] by moving free right-most pointer back to 0
    if( mVectorSize++ >= 1)
    {
        mpObjPtrs[0] = shiftRightFromPosition(0);
    }

    // 1st element was moved to 2nd by shifting right, place new item at 1st location.
    *mpObjPtrs[0] = element;
}

template <typename TYPE>
const TYPE& VECTOR<TYPE>::front()
{
    return (*this)[0];
}

template <typename TYPE>
const TYPE& VECTOR<TYPE>::back()
{
    return (*this)[mVectorSize-1];
}

template <typename TYPE>
unsigned int VECTOR<TYPE>::size() const
{
    return mVectorSize;
}

template <typename TYPE>
unsigned int VECTOR<TYPE>::capacity() const
{
    return mVectorCapacity;
}

template <typename TYPE>
void VECTOR<TYPE>::reserve(unsigned int theSize)
{
    changeCapacity(theSize);
}

template <typename TYPE>
void VECTOR<TYPE>::setGrowthFactor(int factor)
{
    if(factor > 1)
        mGrowthRate = factor;
}

template <typename TYPE>
int VECTOR<TYPE>::getFirstIndexOf(const TYPE& find)
{
    for(unsigned int i = 0; i < mVectorSize; i++) {
        if(*mpObjPtrs[i] == find) {
            return i;
        }
    }
    return -1;
}

template <typename TYPE>
const TYPE& VECTOR<TYPE>::eraseAt(unsigned int elementNumber)
{
    TYPE* item = 0;
    if(elementNumber < mVectorSize && elementNumber >= 0)
    {
        // Must save the pointer even though we are erasing
        item = (mpObjPtrs[mVectorSize-1] = shiftLeftFromPosition(elementNumber));
        mVectorSize--;
    }
    return 0 == item ? mNullItem : *item;
}

template <typename TYPE>
bool VECTOR<TYPE>::remove(const TYPE&  element)
{
    const int index = getFirstIndexOf(element);
    const bool found = (index >= 0);
    if(found) {
        eraseAt(index);
    }
    return found;
}

template <typename TYPE>
int VECTOR<TYPE>::removeAll(const TYPE&  element)
{
    // Optimize vector::removeAll() ???
    int itemsRemoved = 0;
    while(remove(element)) { itemsRemoved++; }
    return itemsRemoved;
}

template <typename TYPE>
bool VECTOR<TYPE>::replace(const TYPE&  find, const TYPE& replaceWith)
{
    const int index = getFirstIndexOf(find);
    const bool found = (index >= 0);

    if(found) {
        *mpObjPtrs[index] = replaceWith;
    }

    return found;
}

template <typename TYPE>
int VECTOR<TYPE>::replaceAll(const TYPE& find, const TYPE& replaceWith)
{
    int itemsReplaced = 0;
    for(unsigned int i = 0; i < mVectorSize; i++) {
        if(*mpObjPtrs[i] == find) {
            *mpObjPtrs[i] = replaceWith;
            itemsReplaced++;
        }
    }
    return itemsReplaced;
}

template <typename TYPE>
void VECTOR<TYPE>::fill(const TYPE& fillElement)
{
    for(unsigned int i = 0; i < mVectorCapacity; i++) {
        *mpObjPtrs[i] = fillElement;
    }
    mVectorSize = mVectorCapacity;
}

template <typename TYPE>
void VECTOR<TYPE>::fillUnused(const TYPE& fillElement)
{
    for(unsigned int i = mVectorSize; i < mVectorCapacity; i++) {
        *mpObjPtrs[i] = fillElement;
    }
    mVectorSize = mVectorCapacity;
}

template <typename TYPE>
void VECTOR<TYPE>::clear()
{
    mVectorSize = 0;
}

template <typename TYPE>
bool VECTOR<TYPE>::isEmpty()
{
    return (0 == mVectorSize);
}

template <typename TYPE>
void VECTOR<TYPE>::reverse()
{
    for(unsigned int i = 0; i < (mVectorSize/2); i++)
    {
        TYPE* temp = mpObjPtrs[i];
        mpObjPtrs[i] = mpObjPtrs[ (mVectorSize-1-i) ];
        mpObjPtrs[ (mVectorSize-1-i) ] = temp;
    }
}

template <typename TYPE>
const TYPE& VECTOR<TYPE>::rotateLeft()
{
    if(mVectorSize >= 2)
    {
        // Shift right and set shifted element to index 0
        mpObjPtrs[0] = shiftRightFromPosition(0);
    }
    return (*this)[0];
}

template <typename TYPE>
const TYPE& VECTOR<TYPE>::rotateRight()
{
    if(mVectorSize >= 2)
    {
        // Shift left, and set the last element to shifted element.
        mpObjPtrs[ (mVectorSize-1) ] = shiftLeftFromPosition(0);
    }
    return (*this)[0];
}

template <typename TYPE>
TYPE& VECTOR<TYPE>::at(const unsigned int i )
{
    return (*this)[i];
}

template <typename TYPE>
TYPE& VECTOR<TYPE>::operator[](const unsigned int i )
{
    return ( i >= 0 && i < mVectorSize) ? *mpObjPtrs[i] : mNullItem;
}

template <typename TYPE>
const TYPE& VECTOR<TYPE>::operator[](const unsigned int i ) const
{
    return ( i >= 0 && i < mVectorSize) ? *mpObjPtrs[i] : mNullItem;
}



// ******* PRIVATE FUNCTIONS:
template <typename TYPE>
void VECTOR<TYPE>::changeCapacity(unsigned int newSize)
{
    if(newSize < mVectorCapacity)
        return;

    // This is slow code so it is #ifdef'd out
#if 0
    // Allocate new memory and call TYPE's default constructor
    TYPE **newData = new TYPE*[ (newSize) ];
    for(unsigned int i = 0; i < newSize; i++) {
        newData[i] = new TYPE();
    }

    // Copy old memory to new one & delete old memory
    if(mpObjPtrs)
    {
        for(unsigned int i = 0; i < mVectorSize; i++)
        {
            newData[i] = mpObjPtrs[i];
        }
        delete [] mpObjPtrs;
    }

    // Point data to new memory
    mpObjPtrs = newData;
#else
    // Allocate pointers of the datatype
    mpObjPtrs = (TYPE**)realloc(mpObjPtrs, sizeof(TYPE*)*newSize);

    // Allocate new objects ONLY at each of the new pointers
    for(unsigned int i = mVectorSize; i < newSize; i++) {
        mpObjPtrs[i] = new TYPE();
    }
#endif

    mVectorCapacity = newSize;
}

template <typename TYPE>
TYPE* VECTOR<TYPE>::shiftLeftFromPosition(unsigned int pos)
{
    TYPE* leftMostItem = mpObjPtrs[pos];
    if(mVectorSize > 1 && (mVectorSize-pos) > 1)
    {
        // Shift elements left by one.
        for( unsigned int i = pos; i < (mVectorSize-1); i++)
        {
            mpObjPtrs[i] = mpObjPtrs[i+1];
        }
    }
    return leftMostItem;
}

template <typename TYPE>
TYPE* VECTOR<TYPE>::shiftRightFromPosition(unsigned int pos)
{
    // Vector size must be at least 1 before calling this function
    TYPE* rightMostItem = mpObjPtrs[mVectorSize-1];

    // Shift elements right by one.
    for( unsigned int i = mVectorSize-1; i > pos; i--)
    {
        mpObjPtrs[i] = mpObjPtrs[i-1];
    }
    return rightMostItem;
}

#endif /* #ifndef _VECTOR_H__ */
