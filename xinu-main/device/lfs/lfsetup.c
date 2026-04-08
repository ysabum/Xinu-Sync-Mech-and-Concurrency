/* lfsetup.c - lfsetup */

#include <xinu.h>
#include "fsbench.h"

/*------------------------------------------------------------------------
 * lfsetup  -  Set a file's index block and data block for the current
 *             file position (assumes per-file mutex held)
 *------------------------------------------------------------------------
 */
status lfsetup(struct lflcblk *lfptr)
{
    dbid32          dnum;
    ibid32          ibnum;
    struct ldentry *ldptr;
    struct lfiblk  *ibptr;
    uint32          newoffset;
    int32           dindex;

    /* Per-file mutex (lfptr->lfmutex) is assumed held by caller */
    fs_cs_enter();   /* still measure this as a critical section */

    ldptr = lfptr->lfdirptr;
    ibptr = &lfptr->lfiblock;

    if (lfptr->lfibdirty || lfptr->lfdbdirty) {
        lfflush(lfptr);
    }

    ibnum = lfptr->lfinum;

    if (ibnum == LF_INULL) {
        ibnum = ldptr->ld_ilist;

        if (ibnum == LF_INULL) {
            ibnum = lfiballoc();
            lfibclear(ibptr, 0);
            ldptr->ld_ilist = ibnum;
            lfptr->lfibdirty = TRUE;
        } else {
            lfibget(Lf_data.lf_dskdev, ibnum, ibptr);
        }

        lfptr->lfinum = ibnum;

    } else if (lfptr->lfpos < ibptr->ib_offset) {

        ibnum = ldptr->ld_ilist;
        lfibget(Lf_data.lf_dskdev, ibnum, ibptr);
        lfptr->lfinum = ibnum;
    }

    while ((lfptr->lfpos & ~LF_IMASK) > ibptr->ib_offset) {

        ibnum = ibptr->ib_next;

        if (ibnum == LF_INULL) {
            ibnum = lfiballoc();
            ibptr->ib_next = ibnum;
            lfibput(Lf_data.lf_dskdev, lfptr->lfinum, ibptr);
            lfptr->lfinum = ibnum;
            newoffset = ibptr->ib_offset + LF_IDATA;
            lfibclear(ibptr, newoffset);
            lfptr->lfibdirty = TRUE;

        } else {
            lfibget(Lf_data.lf_dskdev, ibnum, ibptr);
            lfptr->lfinum = ibnum;
        }

        lfptr->lfdnum = LF_DNULL;
    }

    dindex = (lfptr->lfpos & LF_IMASK) >> 9;

    dnum = lfptr->lfiblock.ib_dba[dindex];

    if (dnum == LF_DNULL) {
        dnum = lfdballoc((struct lfdbfree *)&lfptr->lfdblock);
        lfptr->lfiblock.ib_dba[dindex] = dnum;
        lfptr->lfibdirty = TRUE;

    } else if (dnum != lfptr->lfdnum) {
        read(Lf_data.lf_dskdev, (char *)lfptr->lfdblock, dnum);
        lfptr->lfdbdirty = FALSE;
    }

    lfptr->lfdnum = dnum;
    lfptr->lfbyte = &lfptr->lfdblock[lfptr->lfpos & LF_DMASK];

    fs_cs_exit();
    return OK;
}
