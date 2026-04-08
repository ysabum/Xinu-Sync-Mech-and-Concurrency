/* lflread.c - lflread */

#include <xinu.h>

/*------------------------------------------------------------------------
 * lflread  -  Read from a previously opened local file
 *------------------------------------------------------------------------
 */
devcall lflread(
      struct dentry *devptr,
      char *buff,
      int32 count
    )
{
    uint32      numread;
    int32       nxtbyte;
    struct lflcblk *lfptr;

    if (count < 0) {
        proctab[currpid].errno = EINVAL;
        return SYSERR;
    }

    /* Obtain per-file lock */
    lfptr = &lfltab[devptr->dvminor];
    wait(lfptr->lfmutex);

    for (numread = 0; numread < count; numread++) {
        nxtbyte = lflgetc(devptr);
        if (nxtbyte == SYSERR) {
            signal(lfptr->lfmutex);
            return SYSERR;
        } else if (nxtbyte == EOF) {
            signal(lfptr->lfmutex);
            if (numread == 0) {
                return EOF;
            } else {
                return numread;
            }
        } else {
            *buff++ = (char)(0xff & nxtbyte);
        }
    }

    signal(lfptr->lfmutex);
    return numread;
}
