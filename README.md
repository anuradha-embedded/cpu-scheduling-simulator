# CPU Scheduling Simulator

> Simulate and compare classic CPU scheduling algorithms with Gantt charts and metrics.

[![CI](https://github.com/anuradha-embedded/cpu-scheduling-simulator/actions/workflows/ci.yml/badge.svg)](https://github.com/anuradha-embedded/cpu-scheduling-simulator/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/platform-linux%20%7C%20macOS-lightgrey.svg)](#building)

## Overview

`cpusim` is a small, dependency-free command-line tool that reproduces the
classic CPU scheduling algorithms taught in operating-systems courses and
relied on as a mental model when reasoning about real schedulers. Given a set of
processes (arrival time, CPU burst, and priority), it runs the chosen
algorithm, prints an ASCII Gantt chart of the resulting timeline, and reports
the standard per-process and aggregate metrics.

The goal is twofold: to be a precise, hand-verifiable reference implementation
(every algorithm is covered by tests asserting textbook results), and to be a
quick experimentation surface — change a quantum, add a process, and instantly
see how average waiting time and CPU utilization shift.

## Features

- Five scheduling policies in one binary:
  - **FCFS** — First-Come, First-Served (non-preemptive)
  - **SJF** — Shortest Job First (non-preemptive)
  - **SRTF** — Shortest Remaining Time First (preemptive SJF)
  - **Round Robin** — configurable time quantum
  - **Priority** — both non-preemptive and preemptive variants
- Per-process metrics: completion, turnaround, waiting, and response times.
- Aggregate metrics: average waiting / turnaround / response time and CPU
  utilization.
- ASCII Gantt chart with a duration-scaled timeline and aligned boundary ticks,
  including explicit `idle` segments when the CPU is unoccupied.
- `--algo all` runs every policy on the same workload and prints a side-by-side
  comparison table.
- Flexible input: load a process table from a file, list processes inline on the
  command line, or combine both.
- No third-party dependencies; standard library only; builds offline with either
  CMake or a plain Makefile.

## Design / Architecture

The project is split into a reusable core library and a thin CLI front end so
the scheduling logic can be unit-tested in isolation.

```
            +------------------+
   argv --> |   main.cpp (CLI) |   argument parsing, dispatch
            +---------+--------+
                      |
        +-------------+--------------+
        |        cpusim_core         |
        |  +----------------------+  |
        |  | process.{hpp,cpp}    |  |  parsing + Process / *Result types
        |  | scheduler.{hpp,cpp}  |  |  the five algorithms + timeline model
        |  | report.{hpp,cpp}     |  |  Gantt chart + metric / comparison tables
        |  +----------------------+  |
        +----------------------------+
```

- **`Process`** — the user-supplied descriptor (`id`, `arrival`, `burst`,
  `priority`).
- **`ScheduleResult`** — the output of a run: an ordered list of `GanttSlice`s
  (the timeline, idle gaps included), per-process `ProcessResult`s, and the
  aggregate averages plus CPU utilization.
- Each scheduler is a pure function `vector<Process> -> ScheduleResult`, which
  keeps them trivially testable and free of global state. Preemptive algorithms
  simulate at one time-unit granularity and then coalesce adjacent slices for a
  compact chart.

## Building

The project builds with C++17 and has no external dependencies. Pick whichever
build system you prefer.

### CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Make

```sh
make            # builds build/cpusim
make test       # builds and runs the unit tests
make clean      # removes the build directory
```

Both honour the `CXX` variable, e.g. `make CXX=g++` or
`cmake -S . -B build -DCMAKE_CXX_COMPILER=g++`.

## Usage

```
cpusim --algo <name> [options]

Algorithms (--algo):
  fcfs        First-Come, First-Served
  sjf         Shortest Job First (non-preemptive)
  srtf        Shortest Remaining Time First (preemptive)
  rr          Round Robin (requires --quantum)
  priority    Priority, non-preemptive (default)
  priority-p  Priority, preemptive
  all         Run every algorithm and print a comparison table

Input (combine freely; at least one required):
  --input <file>               load a process table from a file
  --proc id:arr:burst[:prio]   add one inline process (repeatable)

Options:
  --quantum <N>                time quantum for round robin (default 2)
  -h, --help                   show this help
```

A process file is a whitespace-separated table; blank lines and `#` comments are
ignored:

```
# id  arrival  burst  [priority]
P1   0   5   2
P2   1   3   1
P3   2   8   3
P4   3   6   2
```

### Single algorithm

```
$ cpusim --algo fcfs --proc P1:0:5:2 --proc P2:1:3:1 --proc P3:2:8:3 --proc P4:3:6:2
=== FCFS ===

Gantt chart:
+-------+-----+----------+--------+
|  P1   | P2  |    P3    |   P4   |
0       5     8          16       22

PID     Arrival  Burst Priority Completion Turnaround  Waiting  Response
------------------------------------------------------------------------
P1            0      5        2          5          5        0         0
P2            1      3        1          8          7        4         4
P3            2      8        3         16         14        6         6
P4            3      6        2         22         19       13        13
------------------------------------------------------------------------
Average waiting time   : 5.75
Average turnaround time: 11.25
Average response time  : 5.75
CPU utilization        : 100.00%
```

### Round Robin from a file

```
$ cpusim --algo rr --quantum 3 --input examples/procs.txt
=== Round Robin (q=3) ===

Gantt chart:
+-----+-----+-----+-----+----+-----+-----+-----+----+----+
| P1  | P2  | P3  | P4  | P1 | P5  | P3  | P4  | P5 | P3 |
0     3     6     9     12   14    17    20    23   24   26
...
```

### Compare every algorithm

```
$ cpusim --algo all --input examples/procs.txt
...
=== Comparison ===

Algorithm                       Avg Wait      Avg Turn      Avg Resp     CPU %
------------------------------------------------------------------------------
FCFS                                7.80         13.00          7.80    100.00
SJF (non-preemptive)                6.20         11.40          6.20    100.00
SRTF (preemptive)                   6.00         11.20          5.40    100.00
Round Robin (q=2)                  11.80         17.00          2.60    100.00
Priority (non-preemptive)           6.20         11.40          6.20    100.00
Priority (preemptive)               6.40         11.60          5.00    100.00
```

When a workload leaves the CPU idle, the gap is shown explicitly in the chart
and reflected in the utilization figure (see `examples/idle_gap.txt`).

## Project Structure

```
cpu-scheduling-simulator/
├── CMakeLists.txt
├── Makefile
├── LICENSE
├── README.md
├── include/
│   ├── process.hpp        # Process / result / Gantt-slice types + parsing API
│   ├── scheduler.hpp       # ScheduleResult + the five scheduling functions
│   └── report.hpp          # Gantt chart and metric/comparison rendering
├── src/
│   ├── process.cpp         # inline + file process parsing
│   ├── scheduler.cpp       # algorithm implementations and timeline model
│   ├── report.cpp          # ASCII rendering
│   └── main.cpp            # CLI: argument parsing and dispatch
├── tests/
│   └── test_scheduler.cpp  # assert-based unit tests, wired into ctest
├── examples/
│   ├── procs.txt           # sample process set
│   └── idle_gap.txt        # workload with a deliberate idle gap
└── .github/workflows/ci.yml
```

## Testing

Tests live in `tests/test_scheduler.cpp` and use only `<cassert>` — no external
framework. They feed small, hand-verified process sets to each algorithm and
assert the exact expected averages, completion times, and timeline properties.
`NDEBUG` is explicitly cleared in the test translation unit so assertions remain
active even in a Release build.

```sh
ctest --test-dir build --output-on-failure   # via CMake
make test                                      # via Makefile
```

## Roadmap / Future Work

- Multilevel feedback queue (MLFQ) scheduling.
- I/O bursts and a blocked state, so processes alternate CPU and I/O.
- Optional aging for the priority scheduler to prevent starvation.
- Machine-readable output (CSV / JSON) for charting in external tooling.
- Multi-core / multi-queue simulation.

## License

Released under the MIT License. See [LICENSE](LICENSE) for details.
