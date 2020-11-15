# sys-mon
### Modular system info in one place with less resources used*.
*see [Background](#Background)

#### How to:
##### Building
Just run `make`.
##### Running
The main program is `sys-mon` with a path to config file as an argument. You probably want to run it in the background.
##### Example
`./sys-mon default.conf &`

This will load config located in default.conf, which is something like this:
```
[sys-mon]
name = sys-mon
[modules]
cpu
ram available
```
To read the data you can use functions from client/client.h, or as an example use simple-client

`./simple-client sys-mon`

That should output two lines:
 - total cpu usage percentage
 - available memory in KB
```
4
5293112
```
Each line correspondes to the module specified in the [modules] section of the config file.

More complex config:
```
[sys-mon]
name = sys-mon-genmon

[modules]
cpu 2
ram available
generic /sys/class/hwmon/hwmon0/temp2_input
generic /sys/class/hwmon/hwmon0/temp3_input
generic /sys/bus/cpu/devices/cpu0/cpufreq/cpuinfo_cur_freq
generic /sys/bus/cpu/devices/cpu1/cpufreq/cpuinfo_cur_freq
disk_activity sda r_sectors w_sectors
disk_activity sdb r_sectors w_sectors

```

and output:

```
12 10 15
5286380
34000
32000
2000000
2333000
0 0
0 0
```

And that's it!

## Modules
### cpu
#### Cpu / cores usage in percentage.
##### Config
`cpu [cores]`
##### Description
`[cores]`: number of cores [optional], when specified it will output usage for each core next to the total usage
##### Example config lines
`cpu`

`cpu 4`

### ram
#### Ram usage in KB.
##### Config
`ram ['total' 'free' 'available' 'buffers' 'cached']`
##### Description
`['total' 'free' 'available' 'buffers' 'cached']`: requested ram info (flags), currently supported flags and their order. Those parameters correspond with data in /proc/meminfo [https://man7.org/linux/man-pages/man5/proc.5.html]. **Flag order doesn't matter, the values in the output will follow flag order described above**, for example `ram total available` and `ram available total` will output the same line.
##### Example config lines
`ram total available cached`

`ram available buffers cached`

### disk_activity
##### Config
`disk_activity drive_name [total] ['r_reqs' 'r_merges' 'r_sectors' 'r_ticks' 'w_reqs' 'w_merges' 'w_sectors' 'w_ticks']`
##### Description
`drive_name`: name of block drive (in /dev/) like `sda` or `hdb`, `['total']`: if present, shows raw values instead of the change from previous read, `['r_reqs' 'r_merges' 'r_sectors' 'r_ticks' 'w_reqs' 'w_merges' 'w_sectors' 'w_ticks']`: flags that correspond with:  https://www.kernel.org/doc/Documentation/block/stat.txt). **Flag order doesn't matter, the values in the output will follow flag order described above**, for example `disk_activity sda r_reqs w_reqs` and `disk_activity sda w_reqs r_reqs` will output the same line.

##### Example config lines
`disk_activity sda r_sectors w_sectors`

`disk_activity sdb r_sectors w_sectors`

### generic
#### Reads data from file and prints to the output.
##### Config
`generic path`
##### Description
`path`: full path to file (probably from /sys/ of /proc/)
##### Example config lines
`generic /sys/bus/cpu/devices/cpu0/cpufreq/scaling_cur_freq`

# Background

It's more like a 'proof-of-concept' thing. I wanted a data source for a system monitor (like xfce4-genmon) and have low cpu and memory usage.

Instead of opening multiple processes to gather various data from *sysfs* and *procfs*, we can use a single process. And even in a single process, instead of opening and closing all the file handles, sys-mon keeps all the handles open and use *lseek* insted. Sys-mon uses *shared memory* (shm), semaphores and mutex for synchronization (and requesting data).

Goals:
- all needed data in one place
- ability to use previous reads (needed for example for cpu usage, and disk activity)
- lowest possible cpu usage and memory footprint
- being faster to get the all the data from sys-mon than to open and close all those files (sysfs/procfs) by a client itself
- synced access
- extendable (modules, and config)

Results:
- not highly advanced benchmark utils have been used - the Linux *time* command :P - but is showed that (doing 10^6 iterations in a row) reading data from sys-mon-shm times was faster than opening reading and closing all needed files
- it wasn't *much* faster
- the main bottleneck was kernel that uses slow? [.*]printf function to output data to sysfs/procfs
- a new idea to create a kernel module to be even faster
