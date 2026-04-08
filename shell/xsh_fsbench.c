/* xsh_fsbench.c
* -----------------------------------------------------------------------------
* Shell command to run file system concurrency experiments.
* Usage:
*   fsbench <filename> [workers] [iterations] [mode] [pattern]
*     filename   - file to open (e.g., "testfile")
*     workers    - number of concurrent worker processes
*     iterations - iterations per worker
*     mode       - r | w | rw  (read-only, write-only, read/write)
*     pattern    - seq | rand  (sequential or random access)
* The command:
*   - Calls fs_run_experiment() with the chosen parameters
*   - Dumps metrics via fsmetrics_dump() for offline analysis
* ----------------------------------------------------------------------------- 
*/

#include <xinu.h>
#include <stdio.h>
#include <string.h>
#include "fsbench.h"

shellcmd xsh_fsbench(int nargs, char *args[])
{
    int32 nworkers   = 4;
    int32 iterations = FS_ITERATIONS_DEF;
    char *filename   = "testfile";
    int32 workload   = FS_WORKLOAD_RW;
    int32 pattern    = FS_ACCESS_SEQUENTIAL;

    if (nargs < 2) {
        kprintf("Usage: fsbench <filename> [workers] [iterations] [mode] [pattern]\n");
        kprintf("  mode:    r = read-only, w = write-only, rw = read/write\n");
        kprintf("  pattern: seq = sequential, rand = random\n");
        return SHELL_ERROR;
    }

    filename = args[1];

    if (nargs >= 3) {
        nworkers = atoi(args[2]);
    }

    if (nargs >= 4) {
        iterations = atoi(args[3]);
    }

    if (nargs >= 5) {
        if (strcmp(args[4], "r") == 0) {
            workload = FS_WORKLOAD_READONLY;
        } else if (strcmp(args[4], "w") == 0) {
            workload = FS_WORKLOAD_WRITEONLY;
        } else {
            workload = FS_WORKLOAD_RW;
        }
    }

    if (nargs >= 6) {
        if (strcmp(args[5], "rand") == 0) {
            pattern = FS_ACCESS_RANDOM;
        } else {
            pattern = FS_ACCESS_SEQUENTIAL;
        }
    }

    kprintf("fsbench: file=%s workers=%d iterations=%d mode=%d pattern=%d\n",
            filename, (int)nworkers, (int)iterations, (int)workload, (int)pattern);

    if (fs_run_experiment(nworkers, iterations, filename, workload, pattern) == SYSERR) {
        kprintf("fsbench: experiment setup failed\n");
        return SHELL_ERROR;
    }

    fsmetrics_dump();
    return SHELL_OK;
}
