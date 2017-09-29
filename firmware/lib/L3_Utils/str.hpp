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
 * @brief Provides string class with a small foot-print
 * @ingroup Utilities
 *
 * Version: 01102013    Added eraseFirstWords()
 * Version: 05052013    Added tokenize() to get char* tokens.  Added clearAll().  Fixed str::printf()
 * Version: 02122013    Added support for str memory on a stack (external memory).
 *                      Fix removeAllSpecialChars()
 * Version: 12082012    Remove encryption functions, fix beginsWithIgnoreCase().
 *                      Added operators for str
 * Version: 10062012    Added scanf()
 * Version: 06192012    Initial
 */
#ifndef STR_HPP__
#define STR_HPP__



/**
 * Declare the str object on stack, no memory allocation is performed.
 * @note str object on stack cannot grow, so insertion, printf beyond
 *          the memory available will fail.
 *
 * This provides easy way to declare the string on stack.
 * STR_ON_STACK(my_str, 128) turns into :
 *  char __my_strbuffer[128];
 *  str my_str(__my_strbuffer, sizeof(__my_strbuffer));
 */
#define STR_ON_STACK(name, size)    \
    char __##name##buffer[size];    \
    str name((__##name##buffer), sizeof(__##name##buffer))



/**
 * Custom string class
 * @ingroup Utilities
 *
 * str class can provide many string manipulation functions while
 * maintaining internal memory and allocating more memory when needed.
 *
 * @code
 * str s;
 * s = "Hello World...";
 * s.trimEnd(".");
 * printf("%s", s()); // str becomes: "Hello World"
 *
 * s.erase("World");
 * printf("%s", s()); // str becomes: "Hello "
 * s += "World";
 * printf("%s", s()); // str back to: "Hello World"
 *
 * // Use String class as an integer:
 * str myInt;
 * myInt = 123;
 * myInt++; if(myInt == 124)    // true
 * myInt--; if(myInt == "123")  // true
 * int n = (int)myInt;          // Cast back from string to int, n = 123
 * @endcode
 *
 *
 * You can also printf to this str object:
 * @code
 *  str s;
 *  s.printf("Hello World %u\n", 123);
 * @endcode
 *
 *
 * Parsing (Tokenize) Example:
 * @code
 *      str s;
 *      s = "Hello,World.Parse Example!";
 *      puts(s.getToken(",", true)->c_str()); // Prints: Hello
 *      puts(s.getToken(".")->c_str());       // Prints: World
 *      puts(s.getToken()->c_str());          // Prints: Parse
 *      puts(s.getToken()->c_str());          // Prints Example!
 *      assert(0 == s.getToken());            // No more tokens -> NULL Pointer
 * @endcode
 * Note that the original str s is not destroyed during tokenize operations
 */
class str
{
    public:
        /**
         * @{ \name Static conversion functions from string to int and float
         *          @note To convert this string to int or float, use cast operator: int x = (int)myStr;
         */
        static int toInt(const char* pString);     ///< Static method to convert char* string to integer
        static int toInt(str& s)   { return str::toInt(s()); }
        static float toFloat(const char* pString); ///< Static method to convert char* string to float
        static float toFloat(str& s) { return str::toFloat(s()); }
        /** @} */



        /**
         * @{ \name Constructors (and destructor)
         * External memory constructor will be unable to allocate more memory
         * and will fail to grow the string greater than the size
         */
        str();                      ///< Default constructor
        str(int capacity);          ///< Constructor with initial capacity
        str(const char* pString);   ///< Construct from char* pointer
        str(char *buff, int size);  ///< Construct to use external memory
        str(const str& s);          ///< Copy Constructor
        ~str();                     ///< Destructor
        /** @} */



        int getLen() const;      ///< @returns Number of characters in the string
        int getCapacity() const; ///< @returns the current allocated capacity of str
        bool reserve(int n);     ///< reserves memory to hold n characters
        void clear();            ///< Clears the string by putting NULL terminator at 1st char
        void clearAll();         ///< Zeroes out ALL memory that belongs to this str

        void toLower(); ///< Make every alphabet character lowercase
        void toUpper(); ///< Make every alphabet character uppercase

        /**
         * Similar to printf, but will print data into the string
         * @returns The number of characters printed
         */
        int printf(const char* pFormat, ...);

        /**
         * Similar to scanf but scans from this str
         * @returns The number of parameters successfully parsed
         * @note Using %*s will scan a string and ignore it.
         * @note Deliminator by default is a space, read "sscanf()" documentation online for more references.
         *
         * Example:
         * @code
         *      str s = "Something 10:12 pm";
         *      unsigned hour, min;
         *      char amPm[5] = { 0 };
         *      int parsed = str.scanf("%*s %u:%u %2s", &hour, &min, amPm);
         * @endcode
         */
        int scanf(const char* pFormat, ...);

        /**
         * Perform string tokenization (original copy is destroyed)
         * If you want to get pointers separating the string such as "hello world 123",
         * then you can use this function, however, your original string will be destroyed.
         * @code
         * str myStr = "123 45 6789";
         * char *one, *two, *three;
         * if (myStr.tokenize(" ", 3, &one, &two, &three) == 3) {
         *     // This will print "123 45 6789"
         *     printf("%s %s %s\n", one, two, three);
         * }
         * @endcode
         */
        int tokenize(const char* delimators, int char_ptr_count, ...);

        /**
         * @{ \name Insertion functions
         */
        bool insertAtBeg(const char* pString);
        bool insertAtBeg(const str& s) { return insertAtBeg(s()); }
        bool insertAtEnd(const char* pString);
        bool insertAtEnd(const str& s) { return insertAtEnd(s()); }
        bool insertAt(const int index, const char* pString);
        bool insertAt(const int index, const str& s) { return insertAt(index, s()); }
        /** @} */



        /**
         * @{ \name Append functions
         */
        void append(const char* pString);           ///< Appends constant string pointer
        void append(const str& s) { append(s()); }  ///< Appends another str
        void append(int x);                         ///< Appends integer as characters
        void append(float x);                       ///< Appends float as characters
        void appendAsHex(unsigned int num);         ///< Appends as hexadecimal ie: DEADBEEF
        /** @} */



        /**
         * @{ \name Comparison functions
         * @see See also the overloaded == and != operators
         */
        bool compareTo(const char* pString) const;
        bool compareTo(const str& s) const { return compareTo(s.c_str()); }
        bool compareToIgnoreCase(const char* pString) const;
        bool compareToIgnoreCase(const str& s) const { return compareToIgnoreCase(s()); }
        /** @} */



        /**
         * @{ \name Get Word Index Functions
         */
        int firstIndexOfIgnoreCase(const char* pString) const;
        int firstIndexOfIgnoreCase(const str& s) const { return firstIndexOfIgnoreCase(s()); }

        int firstIndexOf(const char* pString) const;
        int firstIndexOf(const str& s) const { return firstIndexOf(s()); }
        int lastIndexOf(const char* pString) const;
        int lastIndexOf(const str& s) const { return lastIndexOf(s()); }
        /** @} */



        /**
         * @{ \name Functions to check a contained string
         */
        bool contains(const char* pString) const;
        bool contains(const str& s) const { return contains(s()); }
        bool containsIgnoreCase(const char* pString) const;
        bool containsIgnoreCase(const str& s) const { return containsIgnoreCase(s()); }

        int countOf(const char* pString) const;
        int countOf(const str& s) const { return countOf(s()); }

        bool beginsWith(const char* pString) const;
        bool beginsWith(const str& s) const { return endsWith(s()); }
        bool beginsWithIgnoreCase(const char* pString) const;
        bool beginsWithIgnoreCase(const str& s) const { return beginsWithIgnoreCase(s()); }

        /**
         * Checks if the string begins with a whole word.
         * If str = "Hello World", then str.beginsWithWholdWord("Hello") will be true
         * But if str = "HelloWorld" then this will be false because this function will
         * check whole word separated by space char unless the separator itself is provided.
         */
        bool beginsWithWholeWord(const char* pString, char seperator=' ') const;
        bool beginsWithWholeWordIgnoreCase(const char* pString, char seperator=' ') const;

        bool endsWith(const char* pString) const;
        bool endsWith(const str& s) const { return endsWith(s()); }
        bool endsWithIgnoreCase(const char* pString) const;
        bool endsWithIgnoreCase(const str& s) const { return endsWithIgnoreCase(s()); }
        /** @} */



        /**
         * @{ \name Erase Functions
         */
        bool erase(const char* pString);
        bool erase(const str& s) { return erase(s()); }
        bool eraseFirst(int nChars);
        bool eraseLast(int nChars);
        bool eraseCharAt(int index);
        bool eraseAllAfter(int index);
        bool eraseAfter(int index, int nChars);
        bool eraseFirstWords(int words, char separator = ' ');
        int eraseAllSpecialChars(); ///< Erase all characters except alphabets and numerals
        /** @} */



        /**
         * @{ \name Trimming Functions to remove leading or trailing character sets
         * Example: str s("...Hello..;;''");
         * s.trimEnd(".;'"); --> s is now: "...Hello"
         */
        void trimStart(const char* pChars);
        void trimStart(const str& s) { trimStart(s()); }
        void trimEnd(const char* pChars);
        void trimEnd(const str& s) { trimEnd(s()); }
        /** @} */



        /**
         * @{ \name Find & Replace Functions
         */
        bool replaceFirst(const char* pFind, const char* pWith);
        bool replaceLast(const char* pFind, const char* pWith);
        int replaceAll(const char* pFind, const char* pWith);
        /** @} */



        /**
         * @{ \name Sub-string Functions
         * @note These sub-string functions will allocate an internal str object
         *       and each subString() function will return reference to this object
         *       to minimize dynamic memory allocation and avoid copy constructors.
         */
        const str& subString(int fromIndex);
        const str& subString(int fromIndex, int charCount);
        const str& subString(char fromFirstChar);
        const str& subString(const char* fromStr);
        const str& subString(char fromFirstChar, int charCount);
        const str& subString(const char* fromStr, int charCount);
        const str& subString(char fromFirstChar, char toLastChar);
        /** @} */

        /**
         * Tokenize function
         * @param pSplitter  The tokens that mark the end of the token to get.
         * @param restart    Restarts tokenize operation on contents of this str. This optional
         *                     parameter is not needed after restarting 1st tokenize operation.
         * @returns  Pointer to substring, or NULL if no more tokens remain
         * @note     Unlike strtok(), the contents of this str is not destroyed.
         * @warning  The returned substring pointer is temporary, and further calls to this function will
         *            re-use the same substr, so copy the  returned substring if it is to be used later.
         *
         * Example:
         * @code
         *   str s = "Hello,World tokentest";
         *   str* t1 = s.getToken(",", true); // t1 == "Hello"
         *   str* t2 = s.getToken(" ");       // t2 == "World"
         *   str* t3 = s.getToken();          // t3 == "tokentest"
         * @endcode
         */
        const str* getToken(const char* pSplitter = " ", bool restart=false);



        /**
         * @{ \name Datatype detection Functions
         */
        bool isAllAlpha() const;     ///< @returns TRUE if the string is all alphabetical characters only
        bool isAlphaNumeric() const; ///< @returns TRUE if the string is all alpha or numerical characters only
        bool isFloat() const;        ///< @returns TRUE if the string is a floating point number
        bool isUint() const;         ///< @returns TRUE if the string is an unsigned number
        bool isInt() const;          ///< @returns TRUE if the string is a number
        /** @} */


        /**
         * @{ \name Checksum Functions
         */
        unsigned int checksum_Get(); ///< Get integer value of XOR checksum of this string
        void checksum_Append();      ///< Appends checksum characters: Ex: 123 becomes: 123:0A
        void checksum_Remove();      ///< Removes checksum characters: Ex: 123:0A becomes 123
        bool checksum_Verify();      ///< @returns TRUE if for example: actual checksum 0A matches calculated 0A of string "123"
        /** @} */


        /**
         * @{ \name Assignment Operators
         */
        void operator=(const char* pString);  ///< Assign a string: myCStr = "123";
        void operator=(int num);              ///< Assign an int : myCStr = 123;
        void operator=(float num);            ///< Assign a float: myCStr = 1.23;
        str& operator=(const str& rhs);       ///< Assign Operator for str a = str b
        /** @} */


        /**
         * @{ \name Increment & Decrement Operators if the string consist of integer value
         */
        void operator++();           ///< Pre-Increment if String is an integer
        void operator++(int unused); ///< Post-Increment if String is an integer
        void operator--();           ///< Pre-Decrement if String is an integer
        void operator--(int unused); ///< Post-Decrement if String is an integer
        /** @} */


        /**
         * Add/Subtract Operators:
         */
        void operator+=(const char singleChar); ///< Append char using += Operator
        void operator+=(const char* pString);   ///< Append String using += Operator
        void operator+=(const str& s);          ///< Append str to us
        void operator+=(int n);                 ///< Subtract Integer using += Operator if string is an integer value
        void operator-=(const char* pString);   ///< Remove all instances of pString using -= Operator
        void operator-=(const str& s);          ///< Remove (erase) str from us
        void operator-=(int n);                 ///< Subtract Integer String using if string is an integer value
        void operator+=(float n);               ///< Subtract Float using += Operator if string is an float value
        void operator-=(float n);               ///< Subtract Float String using -= Operator if string is an float value
        /** @} */


        /**
         * @{ \name Equality Operators
         */
        bool operator==(const char* pString) const;
        bool operator==(const str& rhs) const;
        bool operator==(int) const;
        bool operator!=(const char* pString) const;
        bool operator!=(const str& rhs) const;
        bool operator!=(int) const;
        // Note: Floats cannot be compared using == operator, they require delta comparison
        /** @} */


        /**
         * @{ \name Comparison Operators to perform comparisons, such as:
         * str s;
         * s = "hello";  if(s < "helloo") ...
         * s = 123;      if(s < 1234) ...
         * s = 1.23;     if(s >= 1.22) ...
         */
        bool operator<(const char* pString) const;
        bool operator<(const str& s) const { return (*this) < s(); }
        bool operator<(int) const;

        bool operator>(const char* pString) const;
        bool operator>(int) const;
        bool operator>(const str& s) const { return (*this) > s(); }

        bool operator<=(const char* pString) const;
        bool operator<=(int) const;
        bool operator<=(const str& s) const { return (*this) <= s(); }

        bool operator>=(const char* pString) const;
        bool operator>=(int) const;
        bool operator>=(const str& s) const { return (*this) >= s(); }
        bool operator<(float) const;
        bool operator>=(float) const;
        bool operator>(float) const;
        bool operator<=(float) const;
        /** @} */


        const char* operator()() const;                   ///< () Operator (without a name): Ex: puts(myCStr());
        const char* c_str() const { return (*this)(); };  ///< Get c-string pointer (calls operator above)
        operator float() const;     ///< (float) Cast Operator: Ex: float x = (float)myCStr;
        operator int() const;       ///< (int) Cast Operator: Ex: int x = (int)myCStr;
        char& operator[](int pos);  ///< Index Operator to get and set value @ Index


    private:
        bool mStackMem;     ///< If we are using memory on stack (cannot reallocate memory)
        int mCapacity;      ///< Capacity of the memory of this string
        char* mpStr;        ///< Pointer to the primary memory
        str* mpTempStr;     ///< Avoid construction of new object for substr functions
        char* mpTokenPtr;   ///< Used for getToken() to remember last token location
        static const int mInvalidIndex = -1;
        static const int mAllocSize = 16;

        /// init() is called by constructors to initialize the string
        void init(int initialLength=mAllocSize)
        {
            mStackMem = false;
            mCapacity = 0;
            mpStr = 0;
            mpTempStr = 0;
            mpTokenPtr = 0;

            reAllocateMem(initialLength);
        }

        /// Ensures that the string contains enough memory to store additional nChars characters
        /// @returns true if successful
        bool ensureMemoryToInsertNChars(const int nChars);

        /// reallocates memory for this string given the new size
        /// @returns true if successful
        bool reAllocateMem(const int size);

        /// copies to our string the string given by pString
        void copyFrom(const char* pString);

        /// Converts single hexadecimal to int, such as "A" to the value of 10
        int singleHexCharToInt(unsigned char theChar);

        /// Converts hexadecimal value of string to its integer equivalent (base 10)
        int hexStrDigitsToInt(char* pString);

        /// For tests :
        friend bool test_str(void);
};

#endif /* STR_HPP__ */
