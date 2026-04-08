/* fsworker.c
* -----------------------------------------------------------------------------
* Worker processes and experiment driver for file system concurrency tests.
* This module:
*  - Defines worker processes that perform repeated file operations
*  - Supports different workload types (read-only, write-only, read/write)
*  - Supports sequential vs random access patterns
*  - Calls into the metrics module to record latency and critical section time
*  - Provides fs_run_experiment() to orchestrate multi-process experiments
* ----------------------------------------------------------------------------- 
*/

#include <xinu.h>
#include <stdio.h>
#include "fsbench.h"


// Forward declaration of internal helpers
static uint32 fs_measure_read(int fd, char *buf, uint32 size,
                              uint32 *cs_ticks, byte *flags);
static uint32 fs_measure_write(int fd, char *buf, uint32 size,
                               uint32 *cs_ticks, byte *flags);

// Simple pseudo-random offset generator for random access
static uint32 fs_rand_offset(uint32 max_size)
{
    uint32 seed = fs_getticks();
    seed = seed * 1103515245 + 12345;
    return (seed % max_size);
}

/*
 * fs_worker
 * --------------------------------------------------------------------------
 * Worker process that performs a configurable number of file operations
 * on a shared file. The worker:
 *  - Opens the file
 *  - Performs ITERATIONS of read/write operations according to workload_type
 *  - Uses either sequential or random access patterns
 *  - Records latency and flags for each operation via fsmetrics_record()
 */
process fs_worker(
    int32  iterations,
    char  *filename,
    int32  workload_type,
    int32  access_pattern)
{
    int   fd;
    char  buffer[128];
    int32 i;
    byte  flags;
    uint32 latency, cs_ticks;

    fd = open(LFILESYS, filename, "rw");
    
    // Record an error sample for open failure
    if (fd == SYSERR) {
        fsmetrics_record(FSOP_OPEN, 0, 0, FS_FLAG_ERROR);
        return SYSERR;
    }

    // Record a trivial open latency sample
    fsmetrics_record(FSOP_OPEN, 0, 0, FS_FLAG_OK);

    for (i = 0; i < iterations; i++) {

        // Optional random access: move file offset before operations
        if (access_pattern == FS_ACCESS_RANDOM) {
            uint32 off = fs_rand_offset(1024); // Example: within first 1KB
            seek(fd, off);
        }

        // Read path
        if (workload_type == FS_WORKLOAD_READONLY ||
            workload_type == FS_WORKLOAD_RW) {

            flags   = FS_FLAG_OK;
            latency = fs_measure_read(fd, buffer, sizeof(buffer),
                                      &cs_ticks, &flags);
            fsmetrics_record(FSOP_READ, latency, cs_ticks, flags);
        }

        // Write path
        if (workload_type == FS_WORKLOAD_WRITEONLY ||
            workload_type == FS_WORKLOAD_RW) {

            flags   = FS_FLAG_OK;
            latency = fs_measure_write(fd, buffer, sizeof(buffer),
                                       &cs_ticks, &flags);
            fsmetrics_record(FSOP_WRITE, latency, cs_ticks, flags);
        }
    }

    // Close and record a close sample
    close(fd);
    fsmetrics_record(FSOP_CLOSE, 0, 0, FS_FLAG_OK);

    return OK;
}

/*
 * fs_measure_read
 * --------------------------------------------------------------------------
 * Measure latency of a single read() call and set flags for edge cases.
 * Also measure critical-section time using fs_cs_snapshot().
 */
static uint32 fs_measure_read(int fd, char *buf, uint32 size,
                              uint32 *cs_ticks, byte *flags)
{
    uint32 start, end;
    uint32 cs_before, cs_after;
    int32  n;

    cs_before = fs_cs_snapshot();
    start     = fs_getticks();
    n         = read(fd, buf, size);
    end       = fs_getticks();
    cs_after  = fs_cs_snapshot();

    *cs_ticks = cs_after - cs_before;

    if (n == SYSERR) {
        *flags |= FS_FLAG_ERROR;
    } else if ((uint32)n < size) {
        *flags |= FS_FLAG_SHORT_IO;
    }

    return (end - start);
}

/*
 * fs_measure_write
 * --------------------------------------------------------------------------
 * Measure latency of a single write() call and set flags for edge cases.
 * Also measure critical-section time using fs_cs_snapshot().
 */
static uint32 fs_measure_write(int fd, char *buf, uint32 size,
                               uint32 *cs_ticks, byte *flags)
{
    uint32 start, end;
    uint32 cs_before, cs_after;
    int32  n;

    cs_before = fs_cs_snapshot();
    start     = fs_getticks();
    n         = write(fd, buf, size);
    end       = fs_getticks();
    cs_after  = fs_cs_snapshot();

    *cs_ticks = cs_after - cs_before;

    if (n == SYSERR) {
        *flags |= FS_FLAG_ERROR;
    } else if ((uint32)n < size) {
        *flags |= FS_FLAG_SHORT_IO;
    }

    return (end - start);
}

/*
 * fs_run_experiment
 * --------------------------------------------------------------------------
 * Orchestrate a full experiment:
 *  - Reset metrics
 *  - Launch nworkers fs_worker processes
 *  - Allow them to run for long enough to complete
 */
syscall fs_run_experiment(
    int32  nworkers,
    int32  iterations,
    char  *filename,
    int32  workload_type,
    int32  access_pattern)
{
    int32 i;

    if (nworkers <= 0 || nworkers > FSMAX_WORKERS) {
        return SYSERR;
    }

    if (iterations <= 0) {
        iterations = FS_ITERATIONS_DEF;
    }

    fsmetrics_reset();
    fs_dirlock_init();

    for (i = 0; i < nworkers; i++) {
        resume(create(fs_worker,
                      4096,          // stack size
                      20,            // priority
                      "fs_worker",   // name
                      4,             // number of args
                      iterations,
                      filename,
                      workload_type,
                      access_pattern));
    }

    sleep(10);

    return OK;
}

