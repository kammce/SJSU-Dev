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
 * @ingroup BoardIO
 */
 
#ifndef STORAGE_HPP__
#define STORAGE_HPP__

#include "src/FileSystemObject.hpp"



/**
 * Storage class contains the File System Objects
 *
 * This class contains the File System Objects for the SD Card and SPI Flash.
 * There are mainly two functionalities provided:
 *      - File IO (Read/Write/Append)
 *      - File System Object manipulation (Mount/Format etc).
 *
 * The FileSystemObject can only be obtained from this class that provides
 * further access to the drive system.
 *
 * @ingroup BoardIO
 */
class Storage
{
    public:
        /// @returns Single Flash Drive Object Reference
        static FileSystemObject& getFlashDrive()
        {
            static FileSystemObject* pFlashDrive = new FileSystemObject(driveNumFlashMem);
            return *pFlashDrive;
        }

        /// @returns Single SD Card Drive Object reference
        static FileSystemObject& getSDDrive()
        {
            static FileSystemObject* pSDCardDrive = new FileSystemObject(driveNumSdCard);
            return *pSDCardDrive;
        }

        /**
         * Copies a file
         * @param pExistingFile  Existing file name
         * @param pNewFile       New file name
         * @param pReadTime         Optional: Provide pointer to get the time taken to read the file
         * @param pWriteTime        Optional: Provide pointer to get the time taken to write the file
         * @param pBytesTransferred Optional: Provide pointer to get number of bytes transferred
         */
        static FRESULT copy(const char* pExistingFile, const char* pNewFile,
                            unsigned int* pReadTime=0, unsigned int* pWriteTime=0,
                            unsigned int* pBytesTransferred=0);

        /**
         * Reads an existing file
         * @param pFilename   The filename to read
         * @param pData       The buffer to save the file data
         * @param bytesToRead Number of bytes to read
         * @param offset      Optional Parameter: file offset to read data from
         */
        static FRESULT read(const char* pFilename,  void* pData, unsigned int bytesToRead, unsigned int offset=0);

        /**
         * Writes an existing file.
         * @param pFilename    The filename to write
         * @param pData        The buffer to write the file data from
         * @param bytesToWrite Number of bytes to write
         * @param offset       Optional Parameter: file offset to write data to file
         */
        static FRESULT write(const char* pFilename, void* pData, unsigned int bytesToWrite, unsigned int offset=0);

        /**
         * Appends an existing file (creates a new file if it doesn't exist)
         * @param pFilename     The filename to append data to
         * @param pData         The buffer to write the file data from
         * @param bytesToAppend Number of bytes to append
         * @param offset        If offset is non-zero, data is written to this offset.
         *                      If offset is not specified or zero, data is appended at the end of the file.
         */
        static FRESULT append(const char* pFilename, const void* pData, unsigned int bytesToAppend, unsigned int offset=0);

    private:
        /// Private constructor to restrict object creation
        Storage() {}
};


#endif /* STORAGE_HPP__ */
