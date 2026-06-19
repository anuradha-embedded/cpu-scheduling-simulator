#ifndef CPUSIM_SCHEDULER_HPP
#define CPUSIM_SCHEDULER_HPP

#include <string>
#include <vector>

#include "process.hpp"

namespace cpusim {

// The complete outcome of scheduling a process set with one algorithm: the
// timeline (Gantt slices, including idle gaps) plus the per-process metrics and
// the aggregate averages.
struct ScheduleResult {
    std::string algorithm;                 // display name of the algorithm used
    std::vector<GanttSlice> timeline;      // ordered, contiguous timeline
    std::vector<ProcessResult> processes;  // one entry per input process

    double avgWaiting = 0.0;
    double avgTurnaround = 0.0;
    double avgResponse = 0.0;
    double cpuUtilization = 0.0;  // busy time / total span, as a percentage

    // Convenience accessor: total span of the timeline (last end - first start).
    int makespan() const;
};

// Each scheduler takes the raw process set and returns a fully populated
// ScheduleResult. The input is taken by value so the implementation may sort
// and mutate its working copy freely.

// First-Come, First-Served: non-preemptive, ordered purely by arrival time.
ScheduleResult scheduleFCFS(std::vector<Process> procs);

// Shortest Job First: non-preemptive; among arrived processes, pick the one
// with the smallest burst.
ScheduleResult scheduleSJF(std::vector<Process> procs);

// Shortest Remaining Time First: preemptive SJF; at every tick the process with
// the least remaining burst runs.
ScheduleResult scheduleSRTF(std::vector<Process> procs);

// Round Robin with a fixed time quantum. A quantum <= 0 is rejected.
ScheduleResult scheduleRoundRobin(std::vector<Process> procs, int quantum);

// Priority scheduling. When preemptive is true a newly arrived higher-priority
// process evicts the running one; otherwise the chosen process runs to
// completion. Ties are broken by arrival order.
ScheduleResult schedulePriority(std::vector<Process> procs, bool preemptive);

// Collapse adjacent timeline slices that belong to the same process. Useful for
// preemptive algorithms whose tick-level simulation emits many one-unit slices.
void coalesceTimeline(std::vector<GanttSlice>& timeline);

}  // namespace cpusim

#endif  // CPUSIM_SCHEDULER_HPP
