# CPU Scheduler (C++)

## Overview
A CPU scheduling simulator written in C++. It demonstrates classic policies (e.g., FCFS, SJF, SRTF, Priority, Round Robin), computes per-process waiting/turnaround times and overall averages, and prints the execution order / timeline.

## Repository Layout
- `main.cpp` — implementation
- `test_data/` — sample inputs

## Build
# Windows
g++ -o cpu_scheduler.exe main.cpp

## Methods

### FCFS (First-Come, First-Served)

* **Type:** Non-preemptive (FIFO); runs processes in arrival order until completion.
* **Pros:** Simplest to implement; near-zero scheduling overhead.

### SJF (Shortest Job First)

* **Type:** Non-preemptive; always run the job with the smallest next CPU burst.
* **Pros:** Minimizes **average waiting time** if burst lengths are known/accurately predicted.

### SRTF (Shortest Remaining Time First)

* **Type:** Preemptive SJF; a newly arrived job with less remaining time can preempt the running job.
* **Pros:** Often best average **wait/turnaround** times among single-CPU policies (ignoring context-switch cost).

### Priority Scheduling

* **Type:** Pick the highest-priority job (preemptive or non-preemptive variants).
* **Pros:** Flexible—can reflect importance/SLAs or I/O-boundedness.

### Round Robin (RR)

* **Type:** Preemptive time slicing with a **quantum** Q; cycle through the ready queue, each job runs up to Q before yielding.
* **Pros:** Fairness and good **responsiveness** for interactive workloads.

