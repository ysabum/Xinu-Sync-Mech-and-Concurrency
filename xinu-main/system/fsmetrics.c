/* fsmetrics.c
* -----------------------------------------------------------------------------
* Kernel-side metrics collection for file system concurrency experiments.
* This module:
*  - Stores per-operation samples (latency, critical section time, flags)
*  - Maintains aggregate statistics
*  - Provides a dump function to export data via the console
* ----------------------------------------------------------------------------- 
*/

#include <xinu.h>
#include <stdio.h>
#include "fsbench.h"

// Global sample buffer
static struct fs_sample fs_samples[FSMAX_SAMPLES];
static uint32           fs_sample_count = 0;

// Aggregate statistics
static uint32 fs_total_ops      = 0;
static uint32 fs_total_latency  = 0;
static uint32 fs_total_cs_ticks = 0;

// Simple protection using interrupt disable/restore for metrics array
static intmask fsmetrics_lock(void)
{
    return disable();
}

static void fsmetrics_unlock(intmask im)
{
    restore(im);
}

// Reset all metrics before a new experiment
void fsmetrics_reset(void)
{
    intmask im = fsmetrics_lock();

    fs_sample_count   = 0;
    fs_total_ops      = 0;
    fs_total_latency  = 0;
    fs_total_cs_ticks = 0;

    fsmetrics_unlock(im);
}

// Record a single operation sample
void fsmetrics_record(byte op, uint32 latency, uint32 cs_ticks, byte flags)
{
    intmask im = fsmetrics_lock();

    if (fs_sample_count < FSMAX_SAMPLES) {
        struct fs_sample *s = &fs_samples[fs_sample_count++];
        s->op       = op;
        s->latency  = latency;
        s->cs_ticks = cs_ticks;
        s->pid      = getpid();
        s->flags    = flags;
    }

    fs_total_ops++;
    fs_total_latency  += latency;
    fs_total_cs_ticks += cs_ticks;

    fsmetrics_unlock(im);
}

// Dump metrics in a CSV-like format for offline analysis
void fsmetrics_dump(void)
{
    uint32 i;

    kprintf("fsbench: %u samples, %u total ops\n",
            fs_sample_count, fs_total_ops);

    if (fs_total_ops > 0) {
        kprintf("fsbench: avg_latency_ticks=%u avg_cs_ticks=%u\n",
                (uint32)(fs_total_latency / fs_total_ops),
                (uint32)(fs_total_cs_ticks / fs_total_ops));
    }

    kprintf("op,pid,latency_ticks,cs_ticks,flags\n");
    for (i = 0; i < fs_sample_count; i++) {
        struct fs_sample *s = &fs_samples[i];
        kprintf("%u,%d,%u,%u,%u\n",
                (uint32)s->op,
                (int)s->pid,
                s->latency,
                s->cs_ticks,
                (uint32)s->flags);
    }
}
