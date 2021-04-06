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
To read the data you can use functions from client/client.h (and linking with client/client.lo), or as an example use simple-client

`./simple-client sys-mon`

That should output two lines:
 - total cpu idle percentage
 - available memory in KB
```
96
5293112
```
Each line correspondes to each module line specified in the [modules] section of the config file.

More complex config:
```
[sys-mon]
name = sys-mon-genmon

[modules]
cpu total_idle idle
ram available
generic /sys/class/hwmon/hwmon0/temp2_input
generic /sys/class/hwmon/hwmon0/temp3_input
generic /sys/class/hwmon/hwmon0/temp4_input
generic /sys/class/hwmon/hwmon0/temp5_input
generic /sys/bus/cpu/devices/cpu0/cpufreq/cpuinfo_cur_freq
generic /sys/bus/cpu/devices/cpu1/cpufreq/cpuinfo_cur_freq
generic /sys/bus/cpu/devices/cpu2/cpufreq/cpuinfo_cur_freq
generic /sys/bus/cpu/devices/cpu3/cpufreq/cpuinfo_cur_freq
disk_activity sda r_ticks w_ticks
disk_activity sdb r_ticks w_ticks
time diff
```

and output:

```
85 85 83 87 85 
5746700 
37000
34000
42000
40000
2000000
2000000
2833000
2333000
0 26 
0 56 
2002
```

And that's it! :)

## Config
#### Config file divided into two sections: `[sys-mon]` and `[modules]`. General settings are in the `[sys-mon]` section.
#### [sys-mon] config:
`name = NAME` - name of sys-mon instance (reading sys-mon data will refer to it) - mandatory

`[auto-update = true|false]` - if present and true, sys-mon data will be updated in the fixed time interval, otherwise all the updates will be invoked by the client

`[update_ms = TIME(ms)]` - auto update interval (only relevant when `auto-update` is true)


When `auto-update` is true, it's possible to read data just by reading shm file located in `/dev/shm/NAME/` like `cat /dev/shm/sys-mon`, though the reading should stop at the first null (0x00) character since in the sys-mon shm file there is some binary data at the end.
Example config:
```
[sys-mon]
name = sys-mon-auto
auto_update = true
update_ms = 1000
[modules]
cpu
ram avail
time diff
```

## Modules
#### Modules sections starts after `[modules]` in the config file.
### cpu
#### Cpu (total / per cores) usage info in percentage.
##### Config
`cpu ['total_user', 'total_nice', 'total_system', 'total_idle', 'total_io', 'total_irq', 'total_softirq', 'total_steal', 'total_guest', 'total_guest_nice', 'user', 'nice', 'system', 'idle', 'io', 'irq', 'softirq', 'steal', 'guest', 'guest_nice']`
##### Description
Those parameters correspond with data in /proc/stat [https://man7.org/linux/man-pages/man5/proc.5.html]. For each flag prefixed with `total_` there will be a single number in the output (overall cpu time spent in specific task in percentage). For each flag *not* prefixed with `total_` where will be a number *for each core*. For example, assuming 4-core cpu, and mudule config `cpu total_idle idle`, the output will consist of 5 numbers, somethig like `83 85 83 81 84`, where the first number is the overall (total) cpu idle percentage (usualy the average of the per core numbers).
**Flag order doesn't matter, the values in the output will follow flag order described above**, for example `cpu total_idle guest` and `cpu guest total_idle` will output the same line.

Then no parameters are provided it will output `total_idle`.
##### Example config lines
`cpu`
`cpu total_idle idle io`
`cpu total_idle total_io`

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
`generic path ['diff']`
##### Description
`path`: full path to file (probably from /sys/ of /proc/)
`'diff'`: if `diff` is presnt the output will the difference between updates
##### Example config lines
`generic /sys/bus/cpu/devices/cpu0/cpufreq/scaling_cur_freq`

`generic /sys/class/net/enp0s25/statistics/rx_bytes diff`

`generic /sys/class/net/enp0s25/statistics/tx_bytes diff`

### time
#### Prints current time / time diff.
##### Config
`time ['current', 'diff']`
##### Description
If param `current` is present it will output UTC time in millis, if `diff` is present it will output time between updates in millis.
Then no parameters are provided it will act as if `current` is provided.
##### Example config lines
`time`

`time diff`

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
