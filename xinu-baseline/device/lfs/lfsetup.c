/* lfsetup.c - lfsetup */

#include <xinu.h>
#include "fsbench.h"

/*------------------------------------------------------------------------
 * lfsetup  -  Set a file's index block and data block for the current
 *             file position (assumes file mutex held)
 *------------------------------------------------------------------------
 */
status lfsetup(
      struct lflcblk  *lfptr    /* Pointer to slave file device */
    )
{
    dbid32      dnum;           /* Data block to fetch          */
    ibid32      ibnum;          /* I-block number during search */
    struct ldentry *ldptr;      /* Ptr to file entry in dir.    */
    struct lfiblk *ibptr;       /* Ptr to in-memory index block */
    uint32      newoffset;      /* Computed data offset         */
    int32       dindex;         /* Index into array in i-block  */

    /* Obtain exclusive access to the directory */
    wait(Lf_data.lf_mutex);
    fs_cs_enter();

    /* Get pointers to in-memory directory, file's entry in the
       directory, and the in-memory index block */
    ldptr = lfptr->lfdirptr;
    ibptr = &lfptr->lfiblock;

    /* If existing index block or data block changed, write to disk */
    if (lfptr->lfibdirty || lfptr->lfdbdirty) {
        lfflush(lfptr);
    }

    ibnum = lfptr->lfinum;

    /* If no index block in memory, load or allocate first index block */
    if (ibnum == LF_INULL) {

        ibnum = ldptr->ld_ilist;

        if (ibnum == LF_INULL) {     /* Empty file: allocate new i-block */
            ibnum = lfiballoc();
            lfibclear(ibptr, 0);
            ldptr->ld_ilist = ibnum;
            lfptr->lfibdirty = TRUE;
        } else {                     /* Nonempty: read first i-block */
            lfibget(Lf_data.lf_dskdev, ibnum, ibptr);
        }

        lfptr->lfinum = ibnum;

    /* If file position moved before current index block, restart chain */
    } else if (lfptr->lfpos < ibptr->ib_offset) {

        ibnum = ldptr->ld_ilist;
        lfibget(Lf_data.lf_dskdev, ibnum, ibptr);
        lfptr->lfinum = ibnum;
    }

    /* Walk index blocks until one covers the current file position */
    while ((lfptr->lfpos & ~LF_IMASK) > ibptr->ib_offset) {

        ibnum = ibptr->ib_next;

        if (ibnum == LF_INULL) {     /* Allocate new index block */
            ibnum = lfiballoc();
            ibptr->ib_next = ibnum;
            lfibput(Lf_data.lf_dskdev, lfptr->lfinum, ibptr);
            lfptr->lfinum = ibnum;
            newoffset = ibptr->ib_offset + LF_IDATA;
            lfibclear(ibptr, newoffset);
            lfptr->lfibdirty = TRUE;

        } else {                     /* Load next index block */
            lfibget(Lf_data.lf_dskdev, ibnum, ibptr);
            lfptr->lfinum = ibnum;
        }

        lfptr->lfdnum = LF_DNULL;    /* Invalidate old data block */
    }

    /* Determine data block index within the index block */
    dindex = (lfptr->lfpos & LF_IMASK) >> 9;

    /* Load or allocate the correct data block */
    dnum = lfptr->lfiblock.ib_dba[dindex];

    if (dnum == LF_DNULL) {          /* Allocate new data block */
        dnum = lfdballoc((struct lfdbfree *)&lfptr->lfdblock);
        lfptr->lfiblock.ib_dba[dindex] = dnum;
        lfptr->lfibdirty = TRUE;

    } else if (dnum != lfptr->lfdnum) { /* Load existing data block */
        read(Lf_data.lf_dskdev, (char *)lfptr->lfdblock, dnum);
        lfptr->lfdbdirty = FALSE;
    }

    lfptr->lfdnum = dnum;

    /* Set pointer to correct byte within data block */
    lfptr->lfbyte = &lfptr->lfdblock[lfptr->lfpos & LF_DMASK];

    fs_cs_exit();
    signal(Lf_data.lf_mutex);

    return OK;
}
