/* fsbench.h
* -----------------------------------------------------------------------------
* Shared definitions for the Xinu file system concurrency benchmark.
* This header defines:
*  - Operation types and workload modes
*  - Metrics structures and flags
*  - Function prototypes for metrics, workers, and experiments
* -----------------------------------------------------------------------------
*/ 

#ifndef _FSBENCH_H_
#define _FSBENCH_H_

#include <file.h>
#include "tsc.h"

#define FSMAX_SAMPLES      4096      // Maximum number of recorded samples
#define FSMAX_WORKERS      32        // Maximum number of concurrent workers
#define FS_ITERATIONS_DEF  1000      // Default iterations per worker

// File system operation types for classification
#define FSOP_OPEN   1
#define FSOP_READ   2
#define FSOP_WRITE  3
#define FSOP_CLOSE  4

// Workload types (read/write mix)
#define FS_WORKLOAD_READONLY   1
#define FS_WORKLOAD_WRITEONLY  2
#define FS_WORKLOAD_RW         3

// Access pattern flags
#define FS_ACCESS_SEQUENTIAL   0
#define FS_ACCESS_RANDOM       1

// Sample flags (for edge cases, errors, etc.)
#define FS_FLAG_OK             0x00
#define FS_FLAG_ERROR          0x01
#define FS_FLAG_SHORT_IO       0x02
#define FS_FLAG_BLOCKED        0x04

// Single sample of an instrumented operation
struct fs_sample {
    byte    op;         // FSOP_* type
    uint32  latency;    // Ticks between start and end
    uint32  cs_ticks;   // Ticks spent in critical section (if measured)
    pid32   pid;        // Worker PID
    byte    flags;      // FS_FLAG_* bits
};

// Metrics API
void   fsmetrics_reset(void);
void   fsmetrics_record(byte op, uint32 latency, uint32 cs_ticks, byte flags);
void   fsmetrics_dump(void);

// Experiment driver API
syscall fs_run_experiment(
    int32  nworkers,
    int32  iterations,
    char  *filename,
    int32  workload_type,
    int32  access_pattern);

// Fine-grained directory lock API (baseline vs variant)
void fs_dirlock_init(void);
void fs_dirlock_acquire(void);
void fs_dirlock_release(void);


/* Xinu does not provide getticks(), so we wrap ctr1000.
 * ctr1000 = milliseconds since boot.
 */
extern uint32 ctr1000;

static inline uint64 fs_getticks(void)
{
    return rdtsc();
}

#endif /* _FSBENCH_H_ */



