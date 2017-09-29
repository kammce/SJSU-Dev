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

//
//  $Id: syscalls.c 351 2009-01-12 03:05:14Z jcw $
//  $Revision: 351 $
//  $Author: jcw $
//  $Date: 2009-01-11 22:05:14 -0500 (Sun, 11 Jan 2009) $
//  $HeadURL: http://tinymicros.com/svn_public/arm/lpc2148_demo/trunk/newlib/syscalls.c $
//

//
//  Support files for GNU libc.  Files in the system namespace go here.
//  Files in the C namespace (ie those that do not start with an underscore) go in .c
//  
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <_ansi.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <reent.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"

#include "ff.h"
#include "rtc.h"
#include "sys_config.h"     // SYS_CFG_MAX_FILES_OPENED
#include "uart0_min.h"
#include "lpc_sys.h"



void *__dso_handle = 0;
static char_func_t g_output_dev_fptr = 0; ///< Function pointer for output function
static char_func_t g_input_dev_fptr = 0;  ///< Function pointer for input function

void sys_set_outchar_func(char_func_t func)
{
    g_output_dev_fptr = func;
}
void sys_set_inchar_func(char_func_t func)
{
    g_input_dev_fptr = func;
}

int _kill(int pid __attribute__ ((unused)), int sig __attribute__ ((unused)))
{
    puts("Unexpected call to kill()");
    while (1)
        ;
    return 0;
}
void _exit(int status)
{
    puts("Unexpected call to exit()");
    while (1)
        ;
}
int _getpid(int n) { return 0; }
int _init(void) {   return 1;   }

#if (!SYS_CFG_ENABLE_CFILE_IO)
int _write(int fd, const char *ptr, int len)
{
    int i = 0;
    if (g_output_dev_fptr)
    {
        for (i = 0; i < len; i++)
        {
            g_output_dev_fptr(*ptr++);
        }
    }
    return len;
}
int _read(int fd, char *ptr, int len)
{
    int i = 0;
    if (g_input_dev_fptr)
    {
        for (i = 0; i < len; i++)
        {
            *ptr++ = g_input_dev_fptr(0);
        }
    }
    return i;
}

void syscalls_init(void)
{
    /* SYS_CFG_ENABLE_CFILE_IO not enabled, do nothing */
}
int _fstat(int fd __attribute__ ((unused)), struct stat *st __attribute__ ((unused)))
{   return 0;   }
int _isatty(int n)
{
    if (n == (int)stdout || n == (int)stdin || n == (int)stderr) {
        return 1;
    }
    else {
        return 0;
    }
}
int _lseek(int fd, int ptr, int dir) { return 0; }
int _open(const char *path, int flags, ...) { return 0; }
int _close(int fd) { return 0; }

#else
#define CFG_FATFS 1

//
//  Forward prototypes
//
int _mkdir_PARAMS(const char *, mode_t);
int _chmod_PARAMS(const char *, mode_t);
int _read_PARAMS(int, char *, int);
int _lseek_PARAMS(int, int, int);
int _write_PARAMS(int, const char *, int);
int _open_PARAMS(const char *, int, ...);
int _close_PARAMS(int);
int _kill_PARAMS(int, int);
void _exit_PARAMS(int);
int _getpid_PARAMS(int);
caddr_t _sbrk_PARAMS(int);
int _fstat_PARAMS(int, struct stat *);
int _stat_PARAMS(const char *, struct stat *);
int _link_PARAMS(void);
int _unlink_PARAMS(const char *);
void _raise_PARAMS(void);
int _gettimeofday_PARAMS(struct timeval *, struct timezone *);
clock_t _times_PARAMS(struct tms *);
int isatty_PARAMS(int);
int _rename_PARAMS(const char *, const char *);
int _system_PARAMS(const char *);
void _sync_PARAMS(void);
void syscallsInit_PARAMS(void);

int rename_PARAMS(const char *, const char *);

#if 0
static int set_errno_PARAMS(int);
static int remap_handle_PARAMS(int);
static int findslot_PARAMS(int);
#endif

//
//  Following is copied from libc/stdio/local.h to check std streams 
//
extern void _EXFUN (__sinit,( struct _reent *));

#define CHECK_INIT(ptr)            \
do                                 \
{                                  \
  if ((ptr) && !(ptr)->__sdidinit) \
  __sinit (ptr);                   \
}                                  \
while (0)

//
//  Adjust our internal handles to stay away from std* handles
//
#define FILE_HANDLE_OFFSET (0x20)

#define MONITOR_STDIN  0
#define MONITOR_STDOUT 1
#define MONITOR_STDERR 2
#define MONITOR_UART0  3
#define MONITOR_UART1  4
#define MONITOR_USB    5
#define MONITOR_FATFS  6

// 
//
//
typedef struct
{
    int handle;
    int pos;
    int flags;
    FIL *fatfsFCB;
} openFiles_t;

//
//
//
#define MAX_OPEN_FILES      (SYS_CFG_MAX_FILES_OPENED + 3) /* +3 to account for STDIN, STDOUT, and STDERR */
static openFiles_t openfiles[MAX_OPEN_FILES];

static int findslot(int fh)
{
    static int slot;
    static int lastfh = -1;

    if ((fh != -1) && (fh == lastfh))
        return slot;

    for (slot = 0; slot < MAX_OPEN_FILES; slot++)
        if (openfiles[slot].handle == fh)
            break;

    lastfh = fh;

    return slot;
}

//
//  Function to convert std(in|out|err) handles to internal versions.  
//
static int remap_handle(int fh)
{
    CHECK_INIT(_REENT);

    if (fh == STDIN_FILENO)
        return MONITOR_STDIN;
    if (fh == STDOUT_FILENO)
        return MONITOR_STDOUT;
    if (fh == STDERR_FILENO)
        return MONITOR_STDERR;

    return fh - FILE_HANDLE_OFFSET;
}

#ifdef CFG_FATFS
static int remap_fatfs_errors(FRESULT f)
{
    switch (f)
    {
        case FR_NO_FILE:
            errno = ENOENT;
            break;
        case FR_NO_PATH:
            errno = ENOENT;
            break;
        case FR_INVALID_NAME:
            errno = EINVAL;
            break;
        case FR_INVALID_DRIVE:
            errno = ENODEV;
            break;
        case FR_DENIED:
            errno = EACCES;
            break;
        case FR_EXIST:
            errno = EEXIST;
            break;
        case FR_NOT_READY:
            errno = EIO;
            break;
        case FR_WRITE_PROTECTED:
            errno = EACCES;
            break;
            //case FR_RW_ERROR        : errno = EIO;      break;
        case FR_NOT_ENABLED:
            errno = EIO;
            break;
        case FR_NO_FILESYSTEM:
            errno = EIO;
            break;
        case FR_INVALID_OBJECT:
            errno = EBADF;
            break;
        default:
            errno = EIO;
            break;
    }

    return -1;
}
#endif

#ifdef CFG_FATFS
static time_t fatfs_time_to_timet(FILINFO *f)
{
    struct tm tm;

    tm.tm_sec = (f->ftime & 0x001f) << 1;
    tm.tm_min = (f->ftime & 0x07e0) >> 5;
    tm.tm_hour = (f->ftime & 0xf800) >> 11;
    tm.tm_mday = (f->fdate & 0x001f);
    tm.tm_mon = ((f->fdate & 0x01e0) >> 5) - 1;
    tm.tm_year = ((f->fdate & 0xfe00) >> 9) + 80;
    tm.tm_isdst = 0;

    return mktime(&tm);
}
#endif

static int set_errno(int errval)
{
    errno = errval;

    return -1;
}

void syscalls_init(void)
{
    static char initialized = 0;
    int slot;

    if (initialized)
        return;

    initialized = 1;

    __builtin_memset(openfiles, 0, sizeof(openfiles));

    for (slot = 0; slot < MAX_OPEN_FILES; slot++)
        openfiles[slot].handle = -1;

    openfiles[0].handle = MONITOR_STDIN;
    openfiles[1].handle = MONITOR_STDOUT;
    openfiles[2].handle = MONITOR_STDERR;
}

int _mkdir(const char *path, mode_t mode __attribute__ ((unused)))
{
#ifdef CFG_FATFS
    FRESULT f;

    if ((f = f_mkdir(path)) != FR_OK)
        return remap_fatfs_errors(f);

    return 0;
#else
    path = path;
    return set_errno (EIO);
#endif
}

int _chmod(const char *path, mode_t mode)
{
#ifdef CFG_FATFS
    FRESULT f;

    if ((f = f_chmod(path, (mode & S_IWUSR) ? 0 : AM_RDO, AM_RDO)) != FR_OK)
        return remap_fatfs_errors(f);

    return 0;
#else
    path = path;
    mode = mode;
    return set_errno (EIO);
#endif
}

int _read(int fd, char *ptr, int len)
{
    int i = 0;
    int fh;
    int slot;
    //portTickType xBlockTime;
    int bytesUnRead = -1;

    if ((slot = findslot(fh = remap_handle(fd))) == MAX_OPEN_FILES)
        return set_errno(EBADF);

    if (openfiles[slot].flags & O_WRONLY)
        return set_errno(EBADF);

    //xBlockTime = (openfiles[slot].flags & O_NONBLOCK) ? 0 : portMAX_DELAY;
    // Convert stdout and stdin to be MONITOR_STDOUT to make the case statement work.
    if (fd == (int)stdin) {
        fd = MONITOR_STDIN;
    }

    switch (fh)
    {
        case MONITOR_STDIN:
        {
            if (g_input_dev_fptr)
            {
                for (i = 0; i < len; i++)
                {
                    *ptr++ = g_input_dev_fptr(0);
                }
            }

            bytesUnRead = len - i;
        }
            break;

        case MONITOR_STDOUT:
        case MONITOR_STDERR:
            break;

        case MONITOR_UART0:
        case MONITOR_UART1:
        case MONITOR_USB:
        {
        }
            break;

        default:
        {
#ifdef CFG_FATFS
            if (openfiles[slot].fatfsFCB)
            {
                FRESULT f;
                UINT fatfsBytesRead;

                if ((f = f_read(openfiles[slot].fatfsFCB, ptr, len,
                        &fatfsBytesRead)) != FR_OK)
                    return remap_fatfs_errors(f);

                bytesUnRead = len - fatfsBytesRead;
            }
#else
            return set_errno (EIO);
#endif
        }
            break;
    }

    if (bytesUnRead < 0)
        return -1;

    openfiles[slot].pos += len - bytesUnRead;

    return len - bytesUnRead;
}

int _lseek(int fd, int ptr, int dir)
{
    int fh;
    int slot;
    FRESULT f = FR_INVALID_OBJECT;

    if (((slot = findslot(fh = remap_handle(fd))) == MAX_OPEN_FILES)
            || !openfiles[slot].fatfsFCB)
        return set_errno(EBADF);

#ifdef CFG_FATFS
    if (dir == SEEK_SET)
        f = f_lseek(openfiles[slot].fatfsFCB, ptr);
    else if (dir == SEEK_CUR)
        f = f_lseek(openfiles[slot].fatfsFCB,
                openfiles[slot].fatfsFCB->fptr + ptr);
    else if (dir == SEEK_END)
        f = f_lseek(openfiles[slot].fatfsFCB,
                openfiles[slot].fatfsFCB->fsize + ptr);

    if (f != FR_OK)
        return remap_fatfs_errors(f);

    return openfiles[slot].pos = openfiles[slot].fatfsFCB->fptr;
#else
    ptr = ptr;
    dir = dir;
    f = f;
    return set_errno (EIO);
#endif
}

int _write(int fd, const char *ptr, int len)
{
    int i = 0;
    int fh;
    int slot;
    //portTickType xBlockTime;
    int bytesUnWritten = -1;

    if ((slot = findslot(fh = remap_handle(fd))) == MAX_OPEN_FILES)
        return set_errno(EBADF);

    if (openfiles[slot].flags & O_RDONLY)
        return set_errno(EBADF);

    //xBlockTime = (openfiles[slot].flags & O_NONBLOCK) ? 0 : portMAX_DELAY;
    // Convert stdout and stdin to be MONITOR_STDOUT to make the case statement work.
    if (fd == (int)stdout || fd == (int)stderr) {
        fd = MONITOR_STDOUT;
    }

    switch (fh)
    {
        case MONITOR_STDIN:
            break;

        case MONITOR_STDOUT:
        case MONITOR_STDERR:
        {
            if (g_output_dev_fptr)
            {
                for (i = 0; i < len; i++)
                {
                    g_output_dev_fptr(*ptr++);
                }
            }
            bytesUnWritten = len - i;
        }
        break;

        case MONITOR_UART0:
        case MONITOR_UART1:
        case MONITOR_USB:
        {
        }
            break;

        default:
        {
#ifdef CFG_FATFS
            if (openfiles[slot].fatfsFCB)
            {
                FRESULT f;
                UINT fatfsBytesWritten;

                if ((f = f_write(openfiles[slot].fatfsFCB, ptr, len,
                        &fatfsBytesWritten)) != FR_OK)
                    return remap_fatfs_errors(f);

                bytesUnWritten = len - fatfsBytesWritten;
            }
            else
#endif
                return set_errno(EBADF);
        }
            break;
    }

    if (bytesUnWritten == -1 || bytesUnWritten == len)
        return -1;

    openfiles[slot].pos += len - bytesUnWritten;

    return len - bytesUnWritten;
}

int _open(const char *path, int flags, ...)
{
    int fh = 0;
    int slot;

    if ((slot = findslot(-1)) == MAX_OPEN_FILES)
        return set_errno(ENFILE);

    if (flags & O_APPEND)
        flags &= ~O_TRUNC;

    if (!__builtin_strcmp(path, "/dev/uart0"))
        fh = MONITOR_UART0;
    else if (!__builtin_strcmp(path, "/dev/uart1"))
        fh = MONITOR_UART1;
    else if (!__builtin_strcmp(path, "/dev/usb"))
        fh = MONITOR_USB;
    else
    {
#ifdef CFG_FATFS
        UINT fatfsFlags = FA_OPEN_EXISTING;
        FRESULT f;

        //
        // FA_OPEN_EXISTING Opens the file. The function fails if the file is not existing. (Default)
        // FA_OPEN_ALWAYS   Opens the file, if it is existing. If not, the function creates the new file.
        // FA_CREATE_NEW    Creates a new file. The function fails if the file is already existing.
        // FA_CREATE_ALWAYS Creates a new file. If the file is existing, it is truncated and overwritten.
        //
        // O_CREAT If the file does not exist it will be created.
        // O_EXCL  When used with O_CREAT, if the file already exists it is an error and the open() will fail.
        // O_TRUNC If the file already exists and is a regular file and the open mode allows writing (i.e., is O_RDWR or O_WRONLY) it will be truncated to length 0.
        //
        if (((flags & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC))
                && (flags & (O_RDWR | O_WRONLY)))
            fatfsFlags = FA_CREATE_ALWAYS;
        else if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
            fatfsFlags = FA_OPEN_EXISTING;
        else if ((flags & O_CREAT) == O_CREAT)
            fatfsFlags = FA_OPEN_ALWAYS;
        else if ((flags == O_RDONLY) || (flags == O_WRONLY)
                || (flags == O_RDWR))
            fatfsFlags = FA_OPEN_EXISTING;
        else
            return set_errno(EINVAL);

        if ((flags & O_ACCMODE) == O_RDONLY)
            fatfsFlags |= FA_READ;
        else if ((flags & O_ACCMODE) == O_WRONLY)
            fatfsFlags |= FA_WRITE;
        else if ((flags & O_ACCMODE) == O_RDWR)
            fatfsFlags |= (FA_READ | FA_WRITE);
        else
            return set_errno(EINVAL);

        fh = -1;
        errno = EIO;

        if (!openfiles[slot].fatfsFCB)
            if ((openfiles[slot].fatfsFCB = __builtin_calloc(1, sizeof(FIL))))
                if ((fh =
                        ((f = f_open(openfiles[slot].fatfsFCB, path, fatfsFlags))
                                == FR_OK) ? (slot + MONITOR_FATFS) : -1) == -1)
                    remap_fatfs_errors(f);
#else
        fh = -1;
        errno = EIO;
#endif
    }

    if (fh >= 0)
    {
        openfiles[slot].handle = fh;
        openfiles[slot].pos = 0;
        openfiles[slot].flags = flags;

        if (flags & O_APPEND)
        {
            if (f_lseek(openfiles[slot].fatfsFCB,
                    openfiles[slot].fatfsFCB->fsize) != FR_OK)
                fh = -1;
            else
                openfiles[slot].pos = openfiles[slot].fatfsFCB->fptr;
        }
    }

    if ((fh < 0) && openfiles[slot].fatfsFCB)
    {
        free(openfiles[slot].fatfsFCB);
        openfiles[slot].fatfsFCB = NULL;
    }

    return fh >= 0 ? (fh + FILE_HANDLE_OFFSET) : -1;
}

int _close(int fd)
{
    int slot;

    if ((slot = findslot(remap_handle(fd))) == MAX_OPEN_FILES)
        return set_errno(EBADF);

    openfiles[slot].handle = -1;

#ifdef CFG_FATFS
    if (openfiles[slot].fatfsFCB)
    {
        FRESULT f;

        f = f_close(openfiles[slot].fatfsFCB);
        free(openfiles[slot].fatfsFCB);
        openfiles[slot].fatfsFCB = NULL;

        if (f != FR_OK)
            return remap_fatfs_errors(f);
    }
#endif

    return 0;
}

//
//  Tihs is a problem.  FatFS has no way to go from a file handle back to a file,
//  since file name information isn't stored with the handle.  Tried to think of
//  a good way to handle this, couldn't come up with anything.  For now, it
//  returns ENOSYS (not implemented).
//
int _fstat(int fd __attribute__ ((unused)),
        struct stat *st __attribute__ ((unused)))
{
    return set_errno(ENOSYS);
}

int _stat(const char *fname, struct stat *st)
{
#ifdef CFG_FATFS
    FRESULT f;
    FILINFO fi;

    if ((f = f_stat(fname, &fi)) != FR_OK)
        return remap_fatfs_errors(f);

    __builtin_memset(st, 0, sizeof(*st));

    st->st_mode |= (fi.fattrib & AM_DIR) ? S_IFDIR : S_IFREG;
    st->st_mode |=
            (fi.fattrib & AM_RDO) ? ((S_IRWXU & ~S_IWUSR) | (S_IRWXG & ~S_IWGRP)
                                            | (S_IRWXO & ~S_IWOTH)) : (S_IRWXU
                                            | S_IRWXG | S_IRWXO);
    st->st_size = fi.fsize;
    st->st_ctime = fatfs_time_to_timet(&fi);
    st->st_mtime = st->st_ctime;
    st->st_atime = st->st_ctime;
    st->st_blksize = 512;

    return 0;
#else
    fname = fname;
    st = st;
    return set_errno (EIO);
#endif
}

//
//  FatFS and FAT file systems don't support links
//
int _link(void)
{
    return set_errno(ENOSYS);
}

int _unlink(const char *path)
{
#ifdef CFG_FATFS
    FRESULT f;

    if ((f = f_unlink(path)) != FR_OK)
        return remap_fatfs_errors(f);

    return 0;
#else
    path = path;
    return set_errno (EIO);
#endif
}

void _raise(void)
{
    return;
}

//
//  Return a clock that ticks at 100Hz
//
clock_t _times(struct tms *tp)
{
    clock_t timeval = (clock_t) xTaskGetTickCount();

    if (tp)
    {
        tp->tms_utime = timeval;
        tp->tms_stime = 0;
        tp->tms_cutime = 0;
        tp->tms_cstime = 0;
    }

    return timeval;
}
;

int _isatty(int fd)
{
    if (fd == (int)stdout || fd == (int)stdin || fd == (int)stderr) {
        return 1;
    }
    else {
        return 0;
    }
}

int _system(const char *s)
{
    if (s == NULL)
        return 0;

    return set_errno(ENOSYS);
}

int _rename(const char *oldpath, const char *newpath)
{
#ifdef CFG_FATFS
    FRESULT f;

    if ((f = f_rename(oldpath, newpath)))
        return remap_fatfs_errors(f);

    return 0;
#else
    oldpath = oldpath;
    newpath = newpath;
    return set_errno (EIO);
#endif
}

//
//  Default crossdev -t options for ARM newlib doesn't define HAVE_RENAME, so
//  it's trying to use the link/unlink process.  FatFS doesn't support link(),
//  so override the newlib rename() to make it work correctly.
//
int rename(const char *oldpath, const char *newpath)
{
    return _rename(oldpath, newpath);
}

void _sync(void)
{
#ifdef CFG_FATFS
    int slot;

    for (slot = 0; slot < MAX_OPEN_FILES; slot++)
        if (openfiles[slot].fatfsFCB)
            f_sync(openfiles[slot].fatfsFCB);
#endif
}
#endif /* SYS_CFG_ENABLE_CFILE_IO */
