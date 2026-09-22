# armv7l-userspace-counter

> **Note:** This was a hobby experiment. It pokes at the PMU behind the kernel's
> back has side channel effects. Don't run it on anything with consequence —
> shared machines, production hardware, or anything you care about.

A small Linux loadable kernel module (LKM) that grants **unprivileged userspace
access to the ARMv7-A cycle counter** (`PMCCNTR`) and enables the PMU so the
counter actually runs.

On ARMv7, reading the performance monitor registers from EL0 (user mode) traps
unless `PMUSERENR.EN` is set, and the counters are inert unless `PMCR.E` and the
corresponding bit in `PMCNTENSET` are set. This module performs both steps on
every online CPU at load time, and reverses them at unload.

## Why

`clock_gettime()` and friends cost a vDSO call and have nanosecond-ish
granularity. For micro-benchmarking on ARMv7 boards (Raspberry Pi 2, BeagleBone,
Zynq, ...) reading `PMCCNTR` directly is a handful of cycles and gives exact
cycle counts.

## Requirements

- An **ARMv7-A** (`armv7l`) machine. The module uses `mcr p15, ...` coprocessor
  instructions; it will not build or run on x86, ARMv6, or AArch64.
- Kernel headers / build tree for the running kernel at
  `/lib/modules/$(uname -r)/build`.
- `gcc`, `make`, root privileges to load the module.

On Debian/Raspbian:

```sh
sudo apt install raspberrypi-kernel-headers build-essential   # Raspberry Pi OS
# or
sudo apt install linux-headers-$(uname -r) build-essential    # generic
```

## Build

```sh
make
```

Produces `usercounter.ko`.

## Load / unload

```sh
sudo insmod usercounter.ko
dmesg | tail        # -> "Loading usercounter module..." / "user counters enabled"

sudo rmmod usercounter
```

Optionally install into the module tree:

```sh
sudo make install
sudo depmod -a
sudo modprobe usercounter
```

## Usage from userspace

With the module loaded, read the 32-bit cycle counter directly:

```c
static inline unsigned int rdcycle(void)
{
    unsigned int v;
    /* Read CCNT (PMCCNTR): c9, c13, 0 */
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(v));
    return v;
}

int main(void)
{
    unsigned int start = rdcycle();
    /* ... code under test ... */
    unsigned int end = rdcycle();
    printf("%u cycles\n", end - start);
}
```

Compile with `gcc -O2 -march=armv7-a`.

Notes:

- `PMCCNTR` is **32 bits** and wraps roughly every few seconds at ~1 GHz. Take
  short measurements, or handle wraparound (unsigned subtraction is correct for
  a single wrap).
- The counter is **per-CPU**. Pin the thread (`sched_setaffinity` /
  `taskset`) before comparing two reads; otherwise a migration yields garbage.
- The module does **not** enable the 1:64 divider (`PMCR.D`), so the counter
  counts every cycle.
- Frequency scaling changes cycles-per-second, not cycles-per-instruction.
  Disable cpufreq scaling (`performance` governor) for stable numbers.

## What the module programs

| Register | CP15 encoding | Value on load | Value on unload |
|---|---|---|---|
| `PMUSERENR` (user enable) | `c9, c14, 0` | `1` | `0` |
| `PMCR` (control) | `c9, c12, 0` | `1 \| 16` (`E`, `X`) | `0` |
| `PMCNTENSET` / `PMCNTENCLR` | `c9, c12, 1` / `c9, c12, 2` | `0x8000000f` set | `0x8000000f` cleared |

Applied to every online CPU via `on_each_cpu()`.

## Caveats and known limitations

- **CPU hotplug is not handled.** Only CPUs online at `insmod` time are
  programmed. A core brought online afterwards will fault on a userspace
  `PMCCNTR` read.
- **The cycle counter is not reset** at enable (`PMCR.C` is not set), so the
  first read is an arbitrary value. Always measure a delta.
- **Conflicts with `perf`.** This module takes over the PMU without coordinating
  with the kernel's perf subsystem. Running `perf` concurrently may reprogram
  `PMCR`/`PMCNTENSET` and silently invalidate measurements.
- Enabling `0x8000000f` also switches on event counters 0-3, which are never
  assigned an event and may not exist on all implementations.
- Granting EL0 access to the PMU is a side-channel exposure; do not leave this
  loaded on a multi-tenant or production system.
