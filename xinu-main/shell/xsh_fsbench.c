/* xsh_fsbench.c
 * -----------------------------------------------------------------------------
 * Shell command to run file system concurrency experiments.
 *
 * Usage:
 *   fsbench <filename> [workers] [iterations] [mode] [pattern]
 *     filename   - file to open (e.g., "testfile")
 *     workers    - number of concurrent worker processes
 *     iterations - iterations per worker
 *     mode       - r | w | rw  (read-only, write-only, read/write)
 *     pattern    - seq | rand  (sequential or random access)
 *
 * The command:
 *   - Parses user arguments and selects workload parameters
 *   - Calls fs_run_experiment() to launch worker processes
 *   - Dumps all collected metrics via fsmetrics_dump()
 * ----------------------------------------------------------------------------- 
 */

#include <xinu.h>
#include <stdio.h>
#include <string.h>
#include "fsbench.h"

shellcmd xsh_fsbench(int nargs, char *args[])
{
    // Default experiment parameters //
    int32 nworkers   = 4;                    // number of worker processes
    int32 iterations = FS_ITERATIONS_DEF;    // iterations per worker
    char *filename   = "testfile";           // default file name
    int32 workload   = FS_WORKLOAD_RW;       // default: mixed read/write
    int32 pattern    = FS_ACCESS_SEQUENTIAL; // default: sequential access

    // Require at least a filename //
    if (nargs < 2) {
        kprintf("Usage: fsbench <filename> [workers] [iterations] [mode] [pattern]\n");
        kprintf("  mode:    r = read-only, w = write-only, rw = read/write\n");
        kprintf("  pattern: seq = sequential, rand = random\n");
        return SHELL_ERROR;
    }

    // Parse filename
    filename = args[1];

    // Parse optional worker count
    if (nargs >= 3) {
        nworkers = atoi(args[2]);
    }

    // Parse optional iteration count
    if (nargs >= 4) {
        iterations = atoi(args[3]);
    }

    // Parse workload mode (read/write mix)
    if (nargs >= 5) {
        if (strcmp(args[4], "r") == 0) {
            workload = FS_WORKLOAD_READONLY;
        } else if (strcmp(args[4], "w") == 0) {
            workload = FS_WORKLOAD_WRITEONLY;
        } else {
            workload = FS_WORKLOAD_RW;
        }
    }

    // Parse access pattern (sequential vs random)
    if (nargs >= 6) {
        if (strcmp(args[5], "rand") == 0) {
            pattern = FS_ACCESS_RANDOM;
        } else {
            pattern = FS_ACCESS_SEQUENTIAL;
        }
    }

    // Print experiment configuration for reproducibility
    kprintf("fsbench: file=%s workers=%d iterations=%d mode=%d pattern=%d\n",
            filename, (int)nworkers, (int)iterations, (int)workload, (int)pattern);

    // Launch worker processes and run the experiment
    if (fs_run_experiment(nworkers, iterations, filename, workload, pattern) == SYSERR) {
        kprintf("fsbench: experiment setup failed\n");
        return SHELL_ERROR;
    }

    // Dump all collected latency + critical-section metrics
    fsmetrics_dump();
    return SHELL_OK;
}