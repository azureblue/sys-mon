# sys-mon-shm
Basic system info in shm.

It's more like a 'proof-of-concept' thing. I wanted a data source for a system monitor (like xfce4-genmon) and have low cpu and memory usage.

Insted of opening multiple processes to gather various data from *sysfs* and *procfs*, we can use a single process. And even in a single process, insted of opening and closing all the file handles, sys-mon-shm keeps all the handles open and use *lseek* insted. Sys-mon-shm uses *shared memory* (shm) and semaphores for synchronization (and requesting data).

An example config is in *sys-mon.conf* and a simple client example is in *client/simple-client.c*.

Build: make
Usage: [sudo] ./sys-mon CONFIG_FILE [sys-mon.conf], and then data can be read using ./simple-client

Current modules:
- cpu usage (total, in percentage)
- memory (total/free)
- disk activity (reads/writes reqs/sectors/merges/ticks)
- generic (value specified by providing a source file (like in sysfs / procfs) and it's divisor)

Goals:
- all needed data in one place
- lowest possible cpu usage and memory footprint
- being faster to get the all the data from sys-mon-shm than to open and close all those files (sysfs/procfs) by a client itself
- synced access
- extendable (modules, and config (*sys-mon.conf*))

Results:
- not highly advanced benchmark utils have been used - the Linux *time* command :P - but is showed that (doing 10^6 iterations in a row) reading data from sys-mon-shm times was faster than openning reading and closing all needed files
- it wasn't *much* faster
- the main bottleneck was kernel that uses slow? [.*]printf function to output data to sysfs/procfs
- a new idea to create a kernel module to be even faster
