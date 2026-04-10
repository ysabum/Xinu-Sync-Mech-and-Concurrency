/* lflwrite.c - lflwrite */

#include <xinu.h>

/*------------------------------------------------------------------------
 * lflwrite  --  Write data to a previously opened local disk file
 *------------------------------------------------------------------------
 */
devcall lflwrite(
      struct dentry *devptr,    /* Entry in device switch table */
      char *buff,               /* Buffer holding data to write	*/
      int32 count               /* Number of bytes to write	*/
    )
{
    int32           i;           /* Number of bytes written	*/
    struct lflcblk *lfptr;

    if (count < 0) {
        return SYSERR;
    }

    /* Acquire per-file lock:
     * Ensures exclusive access to lfptr, cached blocks, and file position.
     */
    lfptr = &lfltab[devptr->dvminor];
    wait(lfptr->lfmutex);

    /* Iteratate and write one byte at a time */
    for (i = 0; i < count; i++) {
        if (lflputc(devptr, *buff++) == SYSERR) {
            signal(lfptr->lfmutex);
            return SYSERR;
        }
    }

    signal(lfptr->lfmutex);
    return count;
}

