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

#include "str.hpp"
#include <string.h> // memcpy, strcmp
#include <ctype.h>  // tolower/toupper
#include <stdlib.h> // realloc()
#include <stdio.h>  // sprintf
#include <stdarg.h> // va_args



int str::toInt(const char* pString)     {   return atoi(pString);   }
float str::toFloat(const char* pString) {   return atof(pString);   }

str::str()
{
    init();
}
str::str(int capacity)
{
    init(capacity);
}
str::str(const char* pString)
{
    init(strlen(pString)); // Init with enough memory allocation needed to store pString's length
    copyFrom(pString);
}
/// Cannot call init() for this constructor
str::str(char *buff, int size) :
        mStackMem(true),
        mCapacity(0),
        mpStr(buff),
        mpTempStr(NULL),
        mpTokenPtr(NULL)
{
    mCapacity = (size > 0) ? (size - 1) : 0;
    memset(mpStr, 0, mCapacity);
}
str::~str()
{
    //printf("Delete %u bytes @ %p\n", mCapacity, mpStr);
    if(0 != mpStr && !mStackMem) {
        free(mpStr);
    }
    if (mpTempStr) {
        delete mpTempStr;
    }
}


int str::getLen() const
{
    return strlen(mpStr);
}
int str::getCapacity() const
{
    return mCapacity;
}
bool str::reserve(int n)
{
    if(mCapacity < n)
    {
        return reAllocateMem(n);
    }
    else {
        return true;
    }
}
void str::clear()
{
    *mpStr = '\0';
}
void str::clearAll()
{
    memset(mpStr, 0, mCapacity);
}

void str::toLower()
{
    char* pOurStr = mpStr;
    while(0 != *pOurStr) {
        *pOurStr = tolower(*pOurStr);
        pOurStr++;
    }
}
void str::toUpper()
{
    char* pOurStr = mpStr;
    while(0 != *pOurStr) {
        *pOurStr = toupper(*pOurStr);
        pOurStr++;
    }
}

int str::printf(const char* format, ...)
{
    int len = 0;

    va_list args;
    va_start(args, format);

    while (mpStr)
    {
        va_list args_copy;
        va_copy(args_copy, args);

        // getCapacity() doesn't include NULL
        int mem = getCapacity() + 1;
        len = vsnprintf(mpStr, mem, format, args_copy);
        va_end(args_copy);

        // Output is only written if len is greater than 0 and less than our capacity.
        // Break loop when we could print all the string, or when we cannot reserve memory
        bool break_loop = true;
        if (len >= mem) {
            break_loop = !reserve(len + 1);
        }
        if (break_loop) {
            break;
        }
    }

    va_end(args);
    return len;
}

int str::scanf(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    int parsedParams = vsscanf (mpStr, pFormat, args);
    va_end(args);

    return parsedParams;
}

int str::tokenize(const char* delimators, int char_ptr_count, ...)
{
    va_list args;
    va_start(args, char_ptr_count);

    int token_count = 0;
    char *it = NULL;
    char *token = strtok_r(mpStr, delimators, &it);

    while(token != NULL && char_ptr_count > 0) {
        char **p = va_arg(args, char**);
        if (p) {
            *p = token;
        }

        // Get next token
        token = strtok_r(NULL, delimators, &it);

        --char_ptr_count;
        ++token_count;
    }
    va_end(args);

    return token_count;
}

bool str::insertAtBeg(const char* pString)
{
    return insertAt(0, pString);
}
bool str::insertAtEnd(const char* pString)
{
    bool ok = false;
    if (ensureMemoryToInsertNChars(strlen(pString))) {
        strcat(mpStr, pString);
        ok = true;
    }
    return ok;
}
bool str::insertAt(const int index, const char* pString)
{
    const int newLen = strlen(pString);
    bool ok = false;

    if (ensureMemoryToInsertNChars(newLen)) {
        if(index >= 0 && index <= getLen() && newLen > 0)
        {
            // "Hello", insert at 2 "123" ==> "He123llo"
            void* destToMakeRoom = mpStr + index + newLen;
            void* srcToCopyLeftOverString = mpStr + index;
            const int lenToMove = getLen() - index + 1; // +1 for NULL Char
            memmove(destToMakeRoom, srcToCopyLeftOverString, lenToMove);

            memcpy(mpStr + index, pString, newLen);
            ok = true;
        }
    }
    return ok;
}


void str::append(const char* pString)
{
    insertAtEnd(pString);
}
void str::append(int x)
{
    char intValString[16];
    sprintf(intValString, "%i", x);
    append(intValString);
}

void str::append(float x)
{
    char floatValString[32];
    sprintf(floatValString, "%f", x);
    append(floatValString);
}

void str::appendAsHex(unsigned int num)
{
    char hexVal[16];
    sprintf(hexVal, "%02X", num);
    append(hexVal);
}


bool str::compareTo(const char* pString) const
{
    return (0 == strcmp(pString, mpStr));
}
bool str::compareToIgnoreCase(const char* pString) const
{
    return (0 == strcasecmp(mpStr, pString));
}
bool str::contains(const char* pString) const
{
    return firstIndexOf(pString) != mInvalidIndex;
}


int str::firstIndexOfIgnoreCase(const char* pString) const
{
    int findIndex = mInvalidIndex;
    char* firstChar;

    if(0 != *pString && 0 != *mpStr)
    {
        for(char* pSearchLoc = mpStr; 0 != *pSearchLoc; pSearchLoc = firstChar + 1)
        {
            // First, find first char match ignoring the case.
            if(0 == (firstChar = strchr(pSearchLoc, tolower(*pString)))) {
                if(0 == (firstChar = strchr(pSearchLoc, toupper(*pString)))) {
                    findIndex = mInvalidIndex;
                    break;
                }
            }

            // First char matched, now match remaining string
            char* i2 = (char*)pString + 1;
            findIndex = firstChar - pSearchLoc;

            for(char* i1 = firstChar + 1; *i1 && *i2; i1++, i2++)
            {
                if(tolower(*i1) != tolower(*i2))
                {
                    findIndex = mInvalidIndex;
                    break;
                }
            }

            // If iterator2 reached null char, we matched entire string!
            if(0 == *i2) {
                break;
            }
        };
    }

    return findIndex;
}
bool str::containsIgnoreCase(const char* pString) const
{
    return firstIndexOfIgnoreCase(pString) != mInvalidIndex;
}
int str::firstIndexOf(const char* pString) const
{
    char* pFindPtr = strstr(mpStr, pString);
    return (0 == pFindPtr) ? mInvalidIndex : (pFindPtr - mpStr);
}
int str::lastIndexOf(const char* pString) const
{
    const int lenOfStrToFind = strlen(pString);
    char *pFindPtr = 0;
    char *pLastPtr = 0;

    if (0 != lenOfStrToFind) {
        pFindPtr = strstr(mpStr, pString);
        pLastPtr = pFindPtr;

        while(0 != pFindPtr)
        {
            pLastPtr = pFindPtr;
            pFindPtr = strstr(pFindPtr + lenOfStrToFind, pString);
        }
    }
    return (0 == pLastPtr) ? mInvalidIndex : (pLastPtr - mpStr);
}
int str::countOf(const char* pString) const
{
    const int lenOfStrToFind = strlen(pString);
    char* pFindPtr = strstr(mpStr, pString);
    int count = 0;

    if (0 != lenOfStrToFind) {
        while(0 != pFindPtr)
        {
            count++;
            pFindPtr = strstr(pFindPtr + lenOfStrToFind, pString);
        }
    }

    return count;
}


bool str::beginsWith(const char* pString) const
{
    const int theirLen = strlen(pString);

    // If their length is greater, it can't match our string
    return (getLen() >= theirLen && (0 == strncmp(mpStr, pString, theirLen)));
}
bool str::beginsWithIgnoreCase(const char* pString) const
{
    const int theirLen = strlen(pString);

    // If their length is greater, it can't match our string
    return (getLen() >= theirLen && 0 == strncasecmp(pString, mpStr, theirLen));
}
bool str::beginsWithWholeWord(const char* pString, char seperator) const
{
    // After comparison, the char must be a space or null terminated
    int len = strlen(pString);
    if (len > getLen()) {
        return false;
    }

    const char c = mpStr[len];
    return beginsWith(pString) && (seperator == c || '\0' == c);
}
bool str::beginsWithWholeWordIgnoreCase(const char* pString, char seperator) const
{
    // After comparison, the char must be a space or null terminated
    int len = strlen(pString);
    if (len > getLen()) {
        return false;
    }

    const char c = mpStr[len];
    return beginsWithIgnoreCase(pString) && (seperator == c || '\0' == c);
}


bool str::endsWith(const char* pString) const
{
    bool decision = false;

    const int theirLen = strlen(pString);
    const int ourLen = getLen();
    if(ourLen >= theirLen)
    {
        decision = (0 == strcmp(mpStr+ourLen-theirLen, pString));
    }

    return decision;
}
bool str::endsWithIgnoreCase(const char* pString) const
{
    const int theirLen = strlen(pString);
    const int ourLen = getLen();
    bool decision = (theirLen <= ourLen);

    // If decision is not already false, make actual comparison:
    if(true == decision)
    {
        char* pTheirStr = (char*)pString;
        char* pOurStr = mpStr + ourLen - theirLen;

        return (0 == strcasecmp(pTheirStr, pOurStr));
    }

    return decision;
}


bool str::erase(const char* pString)
{
    const int findIndex = firstIndexOf(pString);
    if(mInvalidIndex != findIndex)
    {
        return eraseAfter(findIndex, strlen(pString));
    }
    return false;
}
bool str::eraseFirst(int nChars)
{
    const int len = getLen();
    bool ok = false;
    if(nChars > 0 && nChars <= len)
    {
        memmove(mpStr, mpStr + nChars, len - nChars + 1);
        ok = true;
    }
    return ok;
}
bool str::eraseLast(int nChars)
{
    const int len = getLen();
    bool ok = false;
    if(nChars <= len)
    {
        mpStr[len-nChars] = '\0';
        ok = true;
    }
    return ok;
}
bool str::eraseCharAt(int index)
{
    return eraseAfter(index, 1);
}
bool str::eraseAllAfter(int index)
{
    bool ok = false;
    if(index >= 0 && index < getLen())
    {
        mpStr[index] = '\0';
        ok = true;
    }
    return ok;
}
bool str::eraseAfter(int index, int nChars)
{
    const int ourLen = getLen();
    bool ok = false;

    if(index >= 0 && index < ourLen && nChars > 0)
    {
        // "012345", index = 2, nChars = 6
        if(nChars > ourLen - index) {
            nChars = ourLen - index;
        }
        memmove(mpStr+index, mpStr+index+nChars, ourLen - index - nChars + 1);
        ok = true;
    }
    return ok;
}
bool str::eraseFirstWords(int words, char separator)
{
    int n = 0;
    int found = 0;

    // test one two
    for (n = 0; mpStr[n] && found != words; n++) {
        if (mpStr[n] == separator) {
            found++;
        }
    }

    if (found == words && mpStr[n]) {
        eraseFirst(n);
    }

    return !!found;
}
int str::eraseAllSpecialChars()
{
    int count = 0;

    // Optimize str::eraseAllSpecialChars() ??
    for(int i = 0; i < getLen(); i++)
    {
        const char thisChar = mpStr[i];
        if(!isalnum(thisChar))
        {
            eraseCharAt(i);
            --i;
            ++count;
        }
    }

    return count;
}


void str::trimStart(const char* pChars)
{
    int numBegCharsToRemove = 0;
    const int ourLen = getLen();

    for(int i = 0; i < ourLen; i++)
    {
        const int len = strlen(pChars);
        bool trimDone = false;

        for(int j = 0 ; j < len; j++)
        {
            if(pChars[j] == mpStr[i]) {
                trimDone = true;
                numBegCharsToRemove++;
                break;
            }
        }

        if(!trimDone) {
            break;
        }
    }

    if(numBegCharsToRemove > 0) {
        eraseFirst(numBegCharsToRemove);
    }
}
void str::trimEnd(const char* pChars)
{
    const int trimLen = strlen(pChars);
    for(int i = getLen() - 1; i >= 0; i--)
    {
        bool trimDone = false;
        for(int j = 0 ; j < trimLen; j++)
        {
            if(pChars[j] == mpStr[i])
            {
                trimDone = true;
                mpStr[i] = '\0';
                break;
            }
        }

        // If no trim took place this iteration, break here
        if(!trimDone) {
            break;
        }
    }
}


bool str::replaceFirst(const char* pFind, const char* pWith)
{
    const int findIndex = firstIndexOf(pFind);
    if(findIndex != mInvalidIndex)
    {
        eraseAfter(findIndex, strlen(pFind));
        insertAt(findIndex, pWith);
        return true;
    }
    return false;
}
bool str::replaceLast(const char* pFind, const char* pWith)
{
    const int findIndex = lastIndexOf(pFind);
    if(findIndex != mInvalidIndex)
    {
        eraseAfter(findIndex, strlen(pFind));
        insertAt(findIndex, pWith);
        return true;
    }
    return false;
}
int str::replaceAll(const char* pFind, const char* pWith)
{
    int count = 0;
    int findIndex = mInvalidIndex;

    while(mInvalidIndex != (findIndex = firstIndexOf(pFind)))
    {
        eraseAfter(findIndex, strlen(pFind));
        insertAt(findIndex, pWith);
        ++count;
    }

    return count;
}


/********
 * All substring functions funnel down to this function which creates
 * the mpTempStr object that is returned as reference
 */
const str& str::subString(int fromIndex, int charCount)
{
    if(0 == mpTempStr) {
        mpTempStr = new str();
    }
    str& ref = *mpTempStr;

    ref.clear();
    if (ref.reserve(getLen())) {
        if(fromIndex >= 0 && fromIndex < getLen() && charCount > 0)
        {
            // Cap the charCount if it is greater than remaining length
            if(charCount > getLen() - fromIndex) {
                charCount = getLen() - fromIndex;
            }

            memcpy(ref.mpStr, mpStr+fromIndex, charCount);
            ref.mpStr[charCount] = '\0';
        }
    }

    return ref;
}
// Calls function: str::subString(int fromIndex, int charCount) (above)
const str& str::subString(int fromIndex)
{
    return subString(fromIndex, getLen());
}
// Calls function: subString(int fromIndex)
const str& str::subString(const char* fromStr)
{
    return subString(firstIndexOf(fromStr));
}
// Calls function: subString(const char* fromStr) (above)
const str& str::subString(char fromFirstChar)
{
    const char s[] = {fromFirstChar, 0x00};
    return subString(s);
}
// Calls subString(int fromIndex, int charCount);
const str& str::subString(const char* fromStr, int charCount)
{
    return subString(firstIndexOf(fromStr), charCount);
}
// Calls function: subString(const char* fromStr, int charCount) (above)
const str& str::subString(char fromFirstChar, int charCount)
{
    const char s[] = {fromFirstChar, 0x00};
    return subString(s, charCount);
}
// Calls function: subString(int fromIndex, int charCount)
const str& str::subString(char fromFirstChar, char toLastChar)
{
    const char fi[] = {fromFirstChar, 0x00};
    const char li[] = {toLastChar, 0x00};
    const int firstIndex = firstIndexOf(fi);
    const int lastIndex  =  lastIndexOf(li);

    return subString(firstIndex, lastIndex - firstIndex + 1);
}

const str* str::getToken(const char* pSplitter, bool restart)
{
    if(restart || 0 == mpTokenPtr) {
        mpTokenPtr = mpStr;
    }

    str* pTokenStr = 0;
    if('\0' != *mpTokenPtr)
    {
        // Get char* pointer where we see any chars of pSplitter
        char* pFindPtr = strpbrk(mpTokenPtr, pSplitter);
        int indexOfFindPtr = mpTokenPtr-mpStr;
        int tokenLength = 0;

        if(0 == pFindPtr)
        {
            // No chars found, set token to the remaining string:
            tokenLength = strlen(mpTokenPtr);
            mpTokenPtr = mpStr + getLen();
        }
        else {
            // Length of token is from previous token ptr to the find pointer
            tokenLength = (pFindPtr - mpTokenPtr);

            // Set next token pointer to the pointer after the pFindPtr
            mpTokenPtr = pFindPtr+1;
        }
        pTokenStr = (str*)&subString(indexOfFindPtr, tokenLength);
    }

    return pTokenStr;
}

bool str::isAllAlpha() const
{
    const int ourLen = getLen();
    bool isAlpha = true;

    for(int i = 0; i < ourLen; i++)
    {
        if(! isalpha(mpStr[i]))
        {
            isAlpha = false;
            break;
        }
    }

    return isAlpha;
}
bool str::isAlphaNumeric() const
{
    const int ourLen = getLen();
    bool isAlphaNumero = true;

    for(int i = 0; i < ourLen; i++)
    {
        const char thisChar = mpStr[i];
        if(!isalnum(thisChar))
        {
            isAlphaNumero = false;
            break;
        }
    }

    return isAlphaNumero;
}

bool str::isFloat() const
{
    const int ourLen = getLen();
    bool strIsAFloat = countOf(".") <= 1;

    if(strIsAFloat)
    {
        for(int i = mpStr[0] == '-' ? 1 : 0; i < ourLen; i++)
        {
            const char thisChar = mpStr[i];
            if(thisChar != '.' && ! isdigit(thisChar))
            {
                strIsAFloat = false;
                break;
            }
        }
    }

    return strIsAFloat;
}

bool str::isUint() const
{
    const int ourLen = getLen();
    bool isAllDigits = true;

    for(int i = 0; i < ourLen; i++)
    {
        const char thisChar = mpStr[i];
        if(! isdigit(thisChar))
        {
            isAllDigits = false;
            break;
        }
    }

    return isAllDigits;
}
bool str::isInt() const
{
    const int ourLen = getLen();
    bool strIsAnInt = true;

    for(int i = mpStr[0] == '-' ? 1 : 0; i < ourLen; i++)
    {
        const char thisChar = mpStr[i];
        if(! isdigit(thisChar))
        {
            strIsAnInt = false;
            break;
        }
    }

    return strIsAnInt;
}

unsigned int str::checksum_Get()
{
    int chkSum = 0;
    const int ourLen = getLen();
    for(int i = 0; i < ourLen; i++) {
        chkSum xor_eq mpStr[i];
    }
    return chkSum;
}

void str::checksum_Append()
{
    const unsigned int chksum = checksum_Get();
    append(":");
    appendAsHex(chksum);
}

void str::checksum_Remove()
{
    eraseAllAfter(lastIndexOf(":"));
}

bool str::checksum_Verify()
{
    bool checksumIsValid = false;
    const int checkSumIndex = lastIndexOf(":");

    if(mInvalidIndex != checkSumIndex)
    {
        mpStr[checkSumIndex] = '\0';
        const unsigned int actualChecksum = hexStrDigitsToInt(mpStr+checkSumIndex+1);
        const unsigned int expectedChecksum = checksum_Get();
        checksumIsValid = (actualChecksum == expectedChecksum);
        mpStr[checkSumIndex] = ':';
    }

    return checksumIsValid;
}



/*****************************************************************/
/******************* Operator Overloading ************************/
/*****************************************************************/
void str::operator=(const char* pString)
{
    copyFrom(pString);
}
void str::operator=(int num)
{
    char buff[32];
    sprintf(buff, "%i", num);
    copyFrom(buff);
}

void str::operator=(float num)
{
    char buff[32];
    sprintf(buff, "%f", num);
    copyFrom(buff);
}

void str::operator++()
{
    (*this) += 1; // Call += Operator Below
}
void str::operator++(int unused)
{
    (*this) += 1; // Call += Operator Below
}

void str::operator--()
{
    (*this) -= 1; // Call -= Operator Below
}
void str::operator--(int unused)
{
    (*this) -= 1; // Call -= Operator Below
}



void str::operator+=(const char* pString)
{
    append(pString);
}
void str::operator+=(const str& s)
{
    append(s());
}
void str::operator+=(const char singleChar)
{
    char s[] = {singleChar, '\0'};
    append(s);
}
void str::operator+=(int n)
{
    if(isInt())
    {
        (*this) = str::toInt(mpStr) + n; // Call Assignment Operator for int
    }
}
void str::operator+=(float n)
{
    if(isFloat())
    {
        (*this) = (float)str::toFloat(mpStr) + n; // Call Assignment Operator for float
    }
}
void str::operator-=(float n)
{
    if(isFloat())
    {
        (*this) = (float)str::toFloat(mpStr) - n; // Call Assignment Operator for float
    }
}
void str::operator-=(const char* pString)
{
    erase(pString);
}
void str::operator-=(const str& s)
{
    erase(s());
}
void str::operator-=(int n)
{
    if(isInt())
    {
        (*this) = str::toInt(mpStr) - n; // Call Assignment Operator for int
    }
}
bool str::operator==(const char* pString) const
{
    return compareTo(pString);
}
bool str::operator==(const str& rhs) const
{
    return compareTo(rhs.c_str());
}
bool str::operator==(int n) const
{
    return str::toInt(mpStr) == n;
}
bool str::operator!=(const char* pString) const
{
    return !((*this) == pString); // Call ==Operator for char*
}
bool str::operator!=(const str& rhs) const
{
    return !((*this) == rhs()); // Call ==Operator for char*
}
bool str::operator!=(int n) const
{
    return !((*this) == n); // Call ==Operator for int
}
/*
// Note: Floats cannot be compared using == operator, they require delta comparison
bool cString::operator==(float n)
{
    return cString::toFloat(mpStr) == n;
}
bool cString::operator!=(float n)
{
    return !((*this) == n); // Call ==Operator for float
}
*/

bool str::operator<(const char* pString) const { return strcmp(mpStr, pString) < 0; }
bool str::operator<(int x) const               { return (int)(*this) < x;           }
bool str::operator>(const char* pString) const { return strcmp(mpStr, pString) > 0; }
bool str::operator>(int x) const               { return (int)(*this) > x;           }
bool str::operator<=(const char* pString) const{ return (strcmp(mpStr, pString) < 0 || strcmp(mpStr, pString) == 0); }
bool str::operator<=(int x) const              { return (int)(*this) <= x;          }
bool str::operator>=(const char* pString) const{ return (strcmp(mpStr, pString) > 0 || strcmp(mpStr, pString) == 0); }
bool str::operator>=(int x) const              { return (int)(*this) >= x;          }
bool str::operator<(float x) const             { return (float)(*this) < x;         }
bool str::operator<=(float x) const            { return (float)(*this) <= x;        }
bool str::operator>=(float x) const            { return (float)(*this) >= x;        }
bool str::operator>(float x) const             { return (float)(*this) > x;         }

const char* str::operator()() const
{
    return mpStr;
}

str::operator int() const
{
    return str::toInt(mpStr);
}

str::operator float() const
{
    return str::toFloat(mpStr);
}

char& str::operator[](int pos)
{
    return pos < getLen() ? mpStr[pos] : mpStr[0];
}

str& str::operator=(const str& copy)
{
    // For the assignment operator, we need to copy string data memory into our memory
    // The rest of the member variables do not matter and they will auto-adjust
    if(this != &copy)
    {
        // Same as: this->copyFrom(copy.get());
        *this = copy();
    }
    return *this;
}

str::str(const str& copy)
{
    // Allocate enough memory first
    this->init(copy.getLen());

    // Simply use assignment operator code above
    *this = copy;
}




// Private
bool str::ensureMemoryToInsertNChars(const int nChars)
{
    const int newLen = nChars;
    const int existingLen = getLen();
    const int requiredMem = newLen + existingLen;

    if(mCapacity < requiredMem)
    {
        // Allocate more so we don't end up doing this upon each character insertion
        return reAllocateMem(requiredMem + 32);
    }
    else {
        return true;
    }
}

bool str::reAllocateMem(const int size)
{
    if (mStackMem) {
        return false;
    }

    // Minimum size is 4 bytes, but we need 1 extra char for NULL
    mCapacity = (0 == size) ? 4 : (1 + size);

    // Align the size to minimize memory fragmentation
    mCapacity = (mCapacity / mAllocSize) * mAllocSize + mAllocSize;

    if(0 == mpStr){
        mpStr = (char*)malloc(mCapacity);
        if (NULL != mpStr) {
            memset(mpStr, 0, mCapacity);
        }
    }
    else {
        mpStr = (char*)realloc(mpStr, mCapacity+1);
    }

    return (NULL != mpStr);
}

void str::copyFrom(const char* pString)
{
    const int strLen = strlen(pString);

    if(strLen > mCapacity) {
        // If we can't allocate memory, only copy up to capacity
        if (!reAllocateMem(strLen)) {
            strncpy(mpStr, pString, mCapacity);
            mpStr[mCapacity] = '\0';
            return;
        }
    }

    strcpy(mpStr, pString);
}

int str::singleHexCharToInt(unsigned char theChar)
{
    switch(theChar) {
        case 'a' ... 'f': theChar -= 'a' + 10; break;
        case 'A' ... 'F': theChar -= 'A' + 10; break;
        default: theChar -= '0'; break;
    }

    return theChar;
}
int str::hexStrDigitsToInt(char* pString)
{
    const char leftChar = pString[0];
    const char rightChar = pString[1];
    return (singleHexCharToInt(leftChar) << 4) + singleHexCharToInt(rightChar);
}







#if 0
#include <assert.h>
bool test_str(void)
{
    // Test constructors
    puts("    Test constructors");
    do {
        str s1;
        assert(s1.getCapacity() == s1.mInitSize);

        str s2(8);
        assert(s2.getCapacity() == 8);
        s2 = "abcdefgh";
        assert(s2 == "abcdefgh");
        assert(s2.getCapacity() == 8);

        str s3("hello");
        assert(s3 == "hello");
        assert(5 == s3.getCapacity());

        str s4 = s3;
        assert(s4 == "hello");
        assert(s4.getCapacity() == 5);
    }while(0);


    puts("    Test length and capacity");
    // Test
    do {
        str s = "123";
        assert(123 == str::toInt("123"));
        assert(123 == str::toInt(s));
    }while(0);

    // Test set 1
    do {
        str s = "123";
        assert(3 == s.getLen());
        assert(3 == s.getCapacity());
        assert(s.reserve(10));
        s = "1234567890";
        assert(10 == s.getLen());
        assert(10 == s.getCapacity());
        s.clear();
        assert(0 == s.getLen());

        s = "1234567890";
        s.toLower();
        assert(s == "1234567890");
        s.toUpper();
        assert(s == "1234567890");
        assert(10 == s.getLen());
        assert(10 == s.getCapacity());
    } while(0);


    puts("    Test printf/scanf");
    // Test set 2
    do {
        str s = "";
        assert(s == "");
        assert(1 == s.getCapacity());

        s.printf("Hello");
        assert(s == "Hello");

        s.printf("%s %i", "Hello", 2);
        assert(s == "Hello 2");

        char b[8] = { 0 };
        int i;
        s.scanf("%s %i", b, &i);
        assert(2 == i);
        assert(0 == strcmp(b, "Hello"));
    } while(0);

    // Test set 3
    puts("    Test insertions");
    do {
        str s = "";
        s.insertAtBeg("456");
        assert(s == "456");
        s.insertAtBeg("123");
        assert(s == "123456");

        s.insertAtEnd("789");
        assert(s == "123456789");

        s.insertAt(0, "0");
        assert(s == "0123456789");
        s.insertAt(10, "a");
        assert(s == "0123456789a");
        s.insertAt(1, ".");
        assert(s == "0.123456789a");
        s.insertAt(11, ".");
        assert(s == "0.123456789.a");

        s.clear();
        s.append("hello ");
        assert(s == "hello ");
        s.append("world");
        assert(s == "hello world");

        s.clear();
        s.append(1.123456f);
        assert(s == "1.123456");
        s.append(100);
        assert(s == "1.123456100");

        s.clear();
        s.appendAsHex(0xDEADBEEF);
        assert(s == "DEADBEEF");
    } while(0);

    puts("    Test comparisons");
    // Test set 4
    do {
        str s;
        assert(s.compareTo(""));
        s = "123";
        assert(!s.compareTo(""));
        assert(s.compareTo("123"));
        assert(s.compareToIgnoreCase("123"));
        s = "aBc";
        assert(!s.compareTo("abc"));
        assert(s.compareToIgnoreCase("abc"));
        assert(s.compareToIgnoreCase("ABC"));

        s = "word index test word";
        assert(s.firstIndexOf("word") == 0);
        assert(s.firstIndexOfIgnoreCase("WORD") == 0);

        assert(s.lastIndexOf("word") == 16);
        assert(s.firstIndexOfIgnoreCase("index") == 5);

        assert(s.contains("word"));
        assert(s.contains(""));
        assert(!s.contains("ddd"));
        assert(s.containsIgnoreCase("TEST"));
        assert(s.countOf("word") == 2);
        assert(s.countOf("test") == 1);
        assert(s.countOf("i") == 1);
        assert(s.countOf("z") == 0);
        assert(s.countOf("") == 0);

        s = "word index test word";
        assert(s.beginsWith("word"));
        assert(s.beginsWith("word"));
        assert(s.beginsWithIgnoreCase("WORD"));
        assert(!s.beginsWith("xord"));
        assert(!s.beginsWithIgnoreCase("XORD"));
        assert(s.beginsWithWholeWord("word"));
        assert(s.beginsWithWholeWordIgnoreCase("WORD"));
        assert(!s.beginsWithWholeWord("words"));
        assert(!s.beginsWithWholeWordIgnoreCase("WORDS"));

        s = "hello world";
        assert(s.endsWith("world"));
        assert(!s.endsWith("e"));
        assert(s.endsWithIgnoreCase("WORLD"));
        assert(!s.endsWithIgnoreCase("HELLO"));
    } while(0);

    puts("    Test erasures");
    // Test set 5
    do {
        str s = "hello world 123";
        assert(!s.erase("zebra"));
        assert(s.erase("hello "));
        assert(s == "world 123");

        s = "Hello World";
        assert(s.eraseAfter(0, 100));
        assert(s == "");

        s = "Hello World";
        assert(!s.eraseAfter(0, 0));
        assert(s == "Hello World");

        s = "Hello World";
        assert(s.eraseAfter(0, 1));
        assert(s == "ello World");

        s = "Hello World";
        assert(s.eraseAfter(5, 6));
        assert(s == "Hello");

        s = "Hello World";
        assert(s.eraseFirst(6));
        assert(s == "World");
        assert(s.eraseFirst(5));
        assert(s == "");
        s = "Hello World";
        assert(!s.eraseFirst(12));
        assert(s.eraseFirst(11));
        assert(s == "");

        s = "Hello World";
        assert(s.eraseLast(6));
        assert(s == "Hello");
        assert(s.eraseLast(5));
        assert(s == "");
        s = "Hello World";
        assert(!s.eraseLast(12));
        assert(s.eraseLast(11));
        assert(s == "");

        s = "Hello World";
        assert(!s.eraseCharAt(-1));
        assert(!s.eraseCharAt(11));
        assert(s.eraseCharAt(10));
        assert(s == "Hello Worl");
        assert(s.eraseCharAt(0));
        assert(s == "ello Worl");

        s = "Hello World";
        assert(!s.eraseAllAfter(11));
        assert(s.eraseAllAfter(10));
        assert(s == "Hello Worl");
        assert(!s.eraseAllAfter(-1));
        assert(s.eraseAllAfter(0));
        assert(s == "");

        s = "Hello World";
        assert(!s.eraseAfter(-1, 1));
        assert(!s.eraseAfter(0, 0));
        assert(s.eraseAfter(0, 1));
        assert(s == "ello World");
        assert(s.eraseAfter(9, 1));
        assert(s == "ello Worl");
        assert(s.eraseAfter(0, 9));
        assert(s == "");

        s = "--H$ell#o ++Wor#%#*#ld --2";
        s.eraseAllSpecialChars();
        assert(s == "HelloWorld2");
    } while(0);


    puts("    Test trimming");
    // Test set 6
    do {
        str s = "Hello World";
        s.trimStart("");
        s.trimStart(".");
        s.trimEnd("");
        s.trimEnd(".");
        assert(s == "Hello World");

        s = "... Hello World;()";
        s.trimStart(".");
        assert(s == " Hello World;()");
        s.trimEnd(";()");
        assert(s == " Hello World");
        s.trimStart(" ");
        assert(s == "Hello World");
    } while(0);

    puts("    Test replacing");
    // Test set 6
    do {
        str s = "Hello Hello World World";
        assert(s.replaceFirst("Hello", "YO"));
        assert(!s.replaceFirst("OO", "YO YO"));
        assert(s == "YO Hello World World");

        assert(s.replaceLast("World", "YO"));
        assert(!s.replaceLast("OO", "YO YO"));
        assert(s == "YO Hello World YO");

        assert(0 == s.replaceAll("YY", "*"));
        assert(2 == s.replaceAll("YO", "*"));
        assert(s == "* Hello World *");
    } while(0);


    puts("    Test substring and tokens");
    // Test set 7 : TO DO Test substring and tokenization
    do {

    } while(0);

    puts("    Test stack string");
    // Test set 8
    do {
        char buff[8];
        str s(&buff[0], sizeof(buff));

        assert(0 == s.getLen());
        assert(sizeof(buff)-1 == s.getCapacity());

        assert(s.reserve(sizeof(buff)-1));
        assert(!s.reserve(sizeof(buff)));
        assert(!s.reserve(sizeof(buff)+1));

        s = "1234567";
        assert(s == "1234567");
        s = "12345678";
        assert(s != "12345678");
        s.printf("Hello");
        assert(s == "Hello");
        s.printf("Hello World");
        assert(s == "Hello W");
        s.printf("123456");
        assert(s == "123456");
        s.printf("1234567");
        assert(s == "1234567");

        assert(!s.insertAt(0, "1"));
        assert(!s.insertAtEnd("1"));
        s.clear();
        assert(s.insertAtEnd("1234567"));
        s.clear();
        assert(s.insertAt(0, "1234567"));
    } while(0);

    return true;
}
#endif

