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
static uint32 fs_measure_read(int fd, char *buf, uint32 size, byte *flags);
static uint32 fs_measure_write(int fd, char *buf, uint32 size, byte *flags);

// Simple pseudo-random offset generator for random access
static uint32 fs_rand_offset(uint32 max_size)
{
    // Very simple LCG-style generator using fs_getticks() as a seed source.
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
    uint32 latency;

    fd = open(NAMESPACE, filename, "rw");
    if (fd == SYSERR) {
        // Record an error sample for open failure
        fsmetrics_record(FSOP_OPEN, 0, 0, FS_FLAG_ERROR);
        return SYSERR;
    }

    // Record a trivial open latency sample (you can instrument inside open()
    // itself if you want true open latency).
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
            latency = fs_measure_read(fd, buffer, sizeof(buffer), &flags);
            fsmetrics_record(FSOP_READ, latency, 0, flags);
        }

        // Write path
        if (workload_type == FS_WORKLOAD_WRITEONLY ||
            workload_type == FS_WORKLOAD_RW) {

            flags   = FS_FLAG_OK;
            latency = fs_measure_write(fd, buffer, sizeof(buffer), &flags);
            fsmetrics_record(FSOP_WRITE, latency, 0, flags);
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
 * Measure latency of a single read() call and set flags for edge cases:
 *  - FS_FLAG_ERROR for SYSERR
 *  - FS_FLAG_SHORT_IO if fewer bytes than requested are read
 */
static uint32 fs_measure_read(int fd, char *buf, uint32 size, byte *flags)
{
    uint32 start, end;
    int32  n;

    start = fs_getticks();
    n     = read(fd, buf, size);
    end   = fs_getticks();

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
 * Measure latency of a single write() call and set flags for edge cases:
 *  - FS_FLAG_ERROR for SYSERR
 *  - FS_FLAG_SHORT_IO if fewer bytes than requested are written
 */
static uint32 fs_measure_write(int fd, char *buf, uint32 size, byte *flags)
{
    uint32 start, end;
    int32  n;

    start = fs_getticks();
    n     = write(fd, buf, size);
    end   = fs_getticks();

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
 *  - (Optionally, you could implement a join mechanism instead of sleep)
 *
 * Parameters:
 *  - nworkers: number of concurrent worker processes
 *  - iterations: number of iterations per worker
 *  - filename: shared file to operate on
 *  - workload_type: FS_WORKLOAD_* (read-only, write-only, read/write)
 *  - access_pattern: FS_ACCESS_SEQUENTIAL or FS_ACCESS_RANDOM
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

    // Initialize fine-grained directory lock (no-op in baseline)
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

    /*
     * Simple synchronization: sleep for a conservative upper bound.
     * For more precise control, you could:
     *  - track worker PIDs and poll their states, or
     *  - implement a barrier or completion counter.
     */
    sleep(10); // Adjust based on iterations and platform speed

    return OK;
}
