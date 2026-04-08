/* lflwrite.c - lflwrite */

#include <xinu.h>

/*------------------------------------------------------------------------
 * lflwrite  --  Write data to a previously opened local disk file
 *------------------------------------------------------------------------
 */
devcall lflwrite(
      struct dentry *devptr,
      char *buff,
      int32 count
    )
{
    int32           i;
    struct lflcblk *lfptr;

    if (count < 0) {
        return SYSERR;
    }

    lfptr = &lfltab[devptr->dvminor];
    wait(lfptr->lfmutex);

    for (i = 0; i < count; i++) {
        if (lflputc(devptr, *buff++) == SYSERR) {
            signal(lfptr->lfmutex);
            return SYSERR;
        }
    }

    signal(lfptr->lfmutex);
    return count;
}

