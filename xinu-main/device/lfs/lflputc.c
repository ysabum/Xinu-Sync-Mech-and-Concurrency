/* lflputc.c - lflputc */

#include <xinu.h>

/*------------------------------------------------------------------------
 * lflputc  -  Write a single byte to an open local file
 *            (assumes caller holds lfptr->lfmutex)
 *------------------------------------------------------------------------
 */
devcall lflputc(
      struct dentry *devptr,    /* Entry in device switch table */
      char          ch          /* Character (byte) to write    */
    )
{
    struct lflcblk *lfptr;      /* Ptr to open file table entry */
    struct ldentry *ldptr;      /* Ptr to file's entry in the   */
                                /*  in-memory directory         */

    /* Caller already holds the file mutex */
    lfptr = &lfltab[devptr->dvminor];

    /* If file is not open, return an error */
    if (lfptr->lfstate != LF_USED) {
        return SYSERR;
    }

    /* Return SYSERR for an attempt to skip bytes beyond the byte
     * that is currently the end of the file
     */
    ldptr = lfptr->lfdirptr;
    if (lfptr->lfpos > ldptr->ld_size) {
        return SYSERR;
    }

    /* If pointer is outside current block, set up new block */
    if (lfptr->lfbyte >= &lfptr->lfdblock[LF_BLKSIZ]) {

        /* Set up block for current file position */
        lfsetup(lfptr);
    }

    /* If appending a byte to the file, increment the file size.
     * Note: comparison might be equal, but should not be greater.
     */
    if (lfptr->lfpos >= ldptr->ld_size) {
        ldptr->ld_size++;
        Lf_data.lf_dirdirty = TRUE;
    }

    /* Place byte in buffer and mark buffer "dirty" */
    *lfptr->lfbyte++ = ch;
    lfptr->lfpos++;
    lfptr->lfdbdirty = TRUE;

    return OK;
}
