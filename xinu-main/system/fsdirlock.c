/* fsdirlock.c
 * -----------------------------------------------------------------------------
 * Optional fine-grained lock for directory metadata.
 * This module provides a simple semaphore-based lock that can be used
 * to protect directory entry updates instead of relying solely on
 * interrupt disabling. This supports the "fine-grained lock" variant
 * described in the methodology.
 * 
 * Note:
 *   - This module was actually not used for this project. 
 * -----------------------------------------------------------------------------
 */

#include <xinu.h>
#include "fsbench.h"

/* Compile-time flag to enable fine-grained directory locking
 * Define FS_FINE_GRAINED_DIRLOCK in your build to enable.
 */
#ifdef FS_FINE_GRAINED_DIRLOCK

static sid32 fs_dir_sem;

// Initialize the directory lock; call from fsinit() or similar.
void fs_dirlock_init(void)
{
    fs_dir_sem = semcreate(1);
}

// Acquire the directory lock before updating directory metadata.
void fs_dirlock_acquire(void)
{
    wait(fs_dir_sem);
}

// Release the directory lock after updating directory metadata.
void fs_dirlock_release(void)
{
    signal(fs_dir_sem);
}

#else  // FS_FINE_GRAINED_DIRLOCK not defined

// Baseline: no-op implementations preserve original behavior.
void fs_dirlock_init(void)      { /* no-op in baseline */ }
void fs_dirlock_acquire(void)   { /* no-op in baseline */ }
void fs_dirlock_release(void)   { /* no-op in baseline */ }

#endif