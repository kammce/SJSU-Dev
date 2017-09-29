#ifndef DISKIOSTRUCTS_H_
#define DISKIOSTRUCTS_H_

#include "integer.h"
typedef unsigned char DSTATUS;  ///< Status of Disk Functions


/// Results of Disk Functions
typedef enum
{
	RES_OK = 0,		/* 0: Successful */
	RES_ERROR,		/* 1: R/W Error */
	RES_WRPRT,		/* 2: Write Protected */
	RES_NOTRDY,		/* 3: Not Ready */
	RES_PARERR		/* 4: Invalid Parameter */
} DRESULT;


#endif /* DISKIOSTRUCTS_H_ */
