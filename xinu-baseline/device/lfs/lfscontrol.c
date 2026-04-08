/* lfscontrol.c - lfscontrol */

#include <xinu.h>
#include "fsbench.h"

/*------------------------------------------------------------------------
 * lfscontrol  -  Provide control functions for a local filesystem master
 *------------------------------------------------------------------------
 */

/*--------------------------------------------------------------------*/
/* XXX                                                                 */
/* We should probably write a few functions to check filename length   */
/* and make the directory present in memory if it isn't already. Also  */
/* we need to be able to tell if a directory on disk "looks" okay.     */
/*--------------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* Return TRUE if the filename at 'name' has an acceptable length. */
/* If it isn't acceptable, set error for the calling process and   */
/* return FALSE.                                                   */
/*-----------------------------------------------------------------*/
static bool8 nlcheck(char *name)
{
    uint32 i;

    for (i = 0; i < LF_NAME_LEN; i++)
        if (name[i] == NULLCH)
            break;
    if (i == 0) {
        proctab[currpid].errno = ENOENT;
        return FALSE;
    }
    if (i >= LF_NAME_LEN) {
        proctab[currpid].errno = ENAMETOOLONG;
        return FALSE;
    }
    return TRUE;
}

/*--------------------------------------------------------------*/
/* Truncate the closed file whose entry is pointed to by ldptr. */
/* Return 1 on success. Otherwise set errno and return 0.       */
/* The directory mutex is assumed to be held.                   */
/*--------------------------------------------------------------*/
static int lftrunc2(struct ldentry *ldptr)
{
    struct lfiblk iblock;
    ibid32 ifree;
    ibid32 firstib;
    ibid32 nextib;
    dbid32 nextdb;
    int32 i;

    if (ldptr->ld_size == 0)
        return OK;

    ifree = Lf_data.lf_dir.lfd_ifree;
    firstib = ldptr->ld_ilist;

    for (nextib = firstib; nextib != ifree; nextib = iblock.ib_next) {
        lfibget(Lf_data.lf_dskdev, nextib, &iblock);

        for (i = 0; i < LF_IBLEN; i++) {
            nextdb = iblock.ib_dba[i];
            if (nextdb != LF_DNULL)
                lfdbfree(Lf_data.lf_dskdev, nextdb);
            iblock.ib_dba[i] = LF_DNULL;
        }

        iblock.ib_offset = 0;

        if (iblock.ib_next == LF_INULL)
            iblock.ib_next = ifree;

        lfibput(Lf_data.lf_dskdev, nextib, &iblock);
    }

    Lf_data.lf_dir.lfd_ifree = firstib;
    Lf_data.lf_dirdirty = TRUE;

    return 1;
}

devcall lfscontrol(
         struct dentry *devptr,
         int32 func,
         int32 arg1,
         int32 arg2
        )
{
    int32 retval;
    char *old, *new;
    struct lfdir *dirptr;
    struct ldentry *ldptr;
    char *nam, *cmp;
    int32 dndx;
    bool8 found;
    int32 i;

    switch (func) {

    case LF_CTL_GETDE:

        wait(Lf_data.lf_mutex);
        fs_cs_enter();

        dirptr = &Lf_data.lf_dir;
        if (Lf_data.lf_dirpresent == FALSE) {
            retval = read(Lf_data.lf_dskdev, (char *)dirptr, LF_AREA_DIR);
            if (retval == SYSERR) {
                fs_cs_exit();
                signal(Lf_data.lf_mutex);
                return SYSERR;
            }
            Lf_data.lf_dirpresent = TRUE;
        }

        if (arg1 < 0 || arg1 >= LF_NUM_DIR_ENT) {
            proctab[currpid].errno = ENOENT;
            fs_cs_exit();
            signal(Lf_data.lf_mutex);
            return SYSERR;
        }

        memcpy((void *)arg2,
               (const void *)&Lf_data.lf_dir.lfd_files[arg1],
               sizeof(struct ldentry));

        fs_cs_exit();
        signal(Lf_data.lf_mutex);
        return OK;

    case LF_CTL_DEL:

        old = (char *)arg1;
        if (nlcheck(old) == FALSE) {
            proctab[currpid].errno = ENAMETOOLONG;
            return SYSERR;
        }

        wait(Lf_data.lf_mutex);
        fs_cs_enter();

        dirptr = &Lf_data.lf_dir;
        if (Lf_data.lf_dirpresent == FALSE) {
            retval = read(Lf_data.lf_dskdev, (char *)dirptr, LF_AREA_DIR);
            if (retval == SYSERR) {
                fs_cs_exit();
                signal(Lf_data.lf_mutex);
                return SYSERR;
            }

            if (dirptr->lfd_magic[0] != 'L' ||
                dirptr->lfd_magic[1] != 'F' ||
                dirptr->lfd_magic[2] != 'S' ||
                dirptr->lfd_magic[3] != 'Y') {

                Lf_data.lf_dirpresent = FALSE;
                proctab[currpid].errno = EBADMAGIC;
                fs_cs_exit();
                signal(Lf_data.lf_mutex);
                return SYSERR;
            }

            Lf_data.lf_dirpresent = TRUE;
        }

        found = FALSE;
        for (i = 0; i < dirptr->lfd_nfiles; i++) {
            ldptr = &dirptr->lfd_files[i];
            nam = old;
            cmp = ldptr->ld_name;
            while (*nam != NULLCH) {
                if (*nam != *cmp)
                    break;
                nam++;
                cmp++;
            }
            if ((*nam == NULLCH) && (*cmp == NULLCH)) {
                found = TRUE;
                break;
            }
        }

        if (found == FALSE) {
            proctab[currpid].errno = ENOENT;
            fs_cs_exit();
            signal(Lf_data.lf_mutex);
            return SYSERR;
        }

        if (lftrunc2(ldptr) == 0) {
            fs_cs_exit();
            signal(Lf_data.lf_mutex);
            return SYSERR;
        }

        if (i < dirptr->lfd_nfiles - 1) {
            struct ldentry *ldptr2;
            ldptr2 = &dirptr->lfd_files[dirptr->lfd_nfiles - 1];
            memcpy((void *)ldptr, (const void *)ldptr2,
                   sizeof(struct ldentry));
            memset((char *)ldptr2, NULLCH, sizeof(struct ldentry));
        } else {
            memset((char *)ldptr, NULLCH, sizeof(struct ldentry));
        }

        Lf_data.lf_dir.lfd_nfiles--;
        Lf_data.lf_dirdirty = TRUE;

        fs_cs_exit();
        signal(Lf_data.lf_mutex);
        return OK;

    case F_CTL_RENAME:

        old = (char *)arg1;
        if (nlcheck(old) == FALSE) {
            proctab[currpid].errno = ENAMETOOLONG;
            return SYSERR;
        }

        new = (char *)arg2;
        if (nlcheck(new) == FALSE) {
            proctab[currpid].errno = ENAMETOOLONG;
            return SYSERR;
        }

        wait(Lf_data.lf_mutex);
        fs_cs_enter();

        dirptr = &Lf_data.lf_dir;
        if (Lf_data.lf_dirpresent == FALSE) {
            retval = read(Lf_data.lf_dskdev, (char *)dirptr, LF_AREA_DIR);
            if (retval == SYSERR) {
                fs_cs_exit();
                signal(Lf_data.lf_mutex);
                return SYSERR;
            }
            Lf_data.lf_dirpresent = TRUE;
        }

        found = FALSE;
        for (i = 0; i < dirptr->lfd_nfiles; i++) {
            ldptr = &dirptr->lfd_files[i];
            nam = old;
            cmp = ldptr->ld_name;
            while (*nam != NULLCH) {
                if (*nam != *cmp)
                    break;
                nam++;
                cmp++;
            }
            if ((*nam == NULLCH) && (*cmp == NULLCH)) {
                found = TRUE;
                break;
            }
        }

        if (found == FALSE) {
            proctab[currpid].errno = ENOENT;
            fs_cs_exit();
            signal(Lf_data.lf_mutex);
            return SYSERR;
        }

        dndx = i;

        found = FALSE;
        for (i = 0; i < dirptr->lfd_nfiles; i++) {
            ldptr = &dirptr->lfd_files[i];
            nam = new;
            cmp = ldptr->ld_name;
            while (*nam != NULLCH) {
                if (*nam != *cmp)
                    break;
                nam++;
                cmp++;
            }
            if ((*nam == NULLCH) && (*cmp == NULLCH)) {
                found = TRUE;
                break;
            }
        }

        if (found == TRUE) {
            proctab[currpid].errno = EEXIST;
            fs_cs_exit();
            signal(Lf_data.lf_mutex);
            return SYSERR;
        }

        ldptr = &dirptr->lfd_files[dndx];
        nam = ldptr->ld_name;
        while (*new)
            *nam++ = *new++;
        *nam = NULLCH;

        Lf_data.lf_dirdirty = TRUE;

        fs_cs_exit();
        signal(Lf_data.lf_mutex);
        return OK;

    case LF_CTL_EXIST:

        wait(Lf_data.lf_mutex);
        fs_cs_enter();

        dirptr = &Lf_data.lf_dir;
        if (Lf_data.lf_dirpresent == FALSE) {
            retval = read(Lf_data.lf_dskdev, (char *)dirptr, LF_AREA_DIR);
            if (retval == SYSERR) {
                fs_cs_exit();
                signal(Lf_data.lf_mutex);
                return SYSERR;
            }
            Lf_data.lf_dirpresent = TRUE;
        }

        if (dirptr->lfd_magic[0] != 'L' ||
            dirptr->lfd_magic[1] != 'F' ||
            dirptr->lfd_magic[2] != 'S' ||
            dirptr->lfd_magic[3] != 'Y') {

            Lf_data.lf_dirpresent = FALSE;
            proctab[currpid].errno = EBADMAGIC;
            fs_cs_exit();
            signal(Lf_data.lf_mutex);
            return SYSERR;
        }

        fs_cs_exit();
        signal(Lf_data.lf_mutex);
        return OK;

    default:
        proctab[currpid].errno = EINVAL;
        return SYSERR;
    }
}
