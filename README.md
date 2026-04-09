# Synchronization Mechanisms and Concurrency Trade-offs in the Xinu File System

This repository contains two modified Xinu kernels for the Intel Galileo platform. In addition to dealing with the possibility that (a) there is no Ethernet link, or (b) there is Ethernet but no DHCP server, or (c) the user uses the netstatus command to shut down the networking, the code extends the Purdue Embedded Xinu distribution with instrumentation and synchronization variants used to study file system concurrency. The project evaluates how Xinu’s locking granularity and interrupt-disabling behavior influence scalability, latency, and contention under concurrent workloads.

The repository includes:

- **xinu-baseline**: the unmodified synchronization model (coarse-grained global directory lock + interrupt-disabled critical sections)

- **xinu-main**: the fine-grained locking variant (per-file semaphore for per-file metadata; global lock only for directory updates)

Both kernels include instrumentation for measuring:

1. Per-operation latency  
2. Time spent inside file-system critical sections  
3. Worker-level behavior under concurrent access  
4. Effects of locking granularity on scalability

These kernels support experiments addressing the following research questions:

1. **Concurrency Scaling (RQ1):** How does increasing the number of concurrent worker processes affect file-operation latency and aggregate throughput?

2. **Access Pattern Effects (RQ2):** How do sequential, random, read-heavy, and write-heavy workloads stress different synchronization paths and shared metadata structure

3. **Interrupt Masking Costs (RQ3):** How much time is spent inside interrupt-disabled critical sections, and how does this correlate with latency spikes under load?s?

4. **Locking Granularity (RQ4):** How does replacing Xinu’s coarse-grained global directory lock with a per-file locking scheme affect scalability and contention?


## Running Experiments

### Baseline Kernel (`xinu-baseline`)
This kernel uses Xinu’s original synchronization model:

- All metadata updates acquire the global directory mutex  
  `Lf_data.lf_mutex`
- All metadata operations execute inside interrupt-disabled critical sections  
  (`fs_cs_enter()` / `fs_cs_exit()`)

After compiling and booting the baseline kernel on the Galileo, run:

```sh
mklfs
echo hi > testfile
fsbench testfile 4 1000 rw rand
```

## Relevant Files
`include/fsbench.h`: instrumentation interfaces
`system/fsdirlock.c`: optional fine-grained directory lock module
`system/fsmetrics.c`: latency + critical-section timing
`system/fsworker.c`: worker process implementation
`system/fscs.c`: global accounting of time spent inside filesystem critical sections (`fs_cs_enter()`, `fs_cs_exit()`, `fs_cs_snapshot()`)  

### Baseline Synchronization (xinu-baseline)
`device/lfs/lfsopen.c`: directory lock acquisition
`device/lfs/lfsetup.c`: metadata traversal under global lock
`device/lfs/lfscontrol.c`: directory operations
`device/lfl/lflread.c`, `lflwrite.c`, `lflseek.c`, `lflclose.c`: per-file operations (no per-file lock)

### Fine-Grained Variant (xinu-main)
`include/lfile.h`: adds sid32 lfmutex to struct lflcblk
`device/lfs/lfsopen.c`: initializes per-file lock
`device/lfs/lfsetup.c`: rewritten to assume per-file lock
`device/lfl/lflread.c`: acquires lfmutex around reads
`device/lfl/lflwrite.c`: acquires lfmutex around writes
`device/lfl/lflseek.c`: already uses per-file lock
`device/lfl/lflclose.c`: uses per-file lock and flushes metadata

### Filesystem Initialization

`device/lfs/lfscreate.c`: creates directory, free lists, and magic number
`shell/xsh_mklfs.c`: user-facing mklfs command