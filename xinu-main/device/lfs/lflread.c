/* lflread.c - lflread */

#include <xinu.h>

/*------------------------------------------------------------------------
 * lflread  -  Read from a previously opened local file
 *------------------------------------------------------------------------
 */
devcall	lflread (
	  struct dentry *devptr,	/* Entry in device switch table */
	  char	*buff,			    /* Buffer to hold bytes		*/
	  int32	count			    /* Max bytes to read		*/
	)
{
	uint32	numread;		/* Number of bytes read		*/
	int32	nxtbyte;		/* Character or SYSERR/EOF	*/

    struct lflcblk *lfptr;

    if (count < 0) {
        proctab[currpid].errno = EINVAL;
        return SYSERR;
    }

    /* Obtain per-file lock */
    lfptr = &lfltab[devptr->dvminor];
    wait(lfptr->lfmutex);

    /* Iterate and use lflgetc to read indivdiual bytes */

    for (numread = 0; numread < count; numread++) {
        nxtbyte = lflgetc(devptr);
        if (nxtbyte == SYSERR) {
            signal(lfptr->lfmutex);
            return SYSERR;
        } else if (nxtbyte == EOF) {    /* EOF before finished */
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
