#ifndef CPUSIM_PROCESS_HPP
#define CPUSIM_PROCESS_HPP

#include <string>
#include <vector>

namespace cpusim {

// A single process descriptor as supplied by the user. The fields mirror the
// classic operating-systems formulation used when teaching scheduling theory.
struct Process {
    std::string id;     // human-readable label, e.g. "P1"
    int arrival = 0;    // time at which the process enters the ready queue
    int burst = 0;      // total CPU time the process requires
    int priority = 0;   // lower value == higher priority (convention)
};

// Per-process timing results produced by a scheduler run. These are derived
// quantities once the schedule (the sequence of CPU allocations) is known.
struct ProcessResult {
    std::string id;
    int arrival = 0;
    int burst = 0;
    int priority = 0;
    int completion = 0;  // wall-clock time at which the process finished
    int turnaround = 0;  // completion - arrival
    int waiting = 0;     // turnaround - burst
    int response = 0;    // first time on CPU - arrival
};

// A contiguous slice of the timeline during which a single process (or the
// idle CPU) held the processor. The Gantt chart is simply a list of these.
struct GanttSlice {
    std::string id;  // process id, or "idle" when the CPU was unoccupied
    int start = 0;
    int end = 0;     // exclusive end; duration == end - start
};

// Parse an inline process specification of the form "id:arrival:burst:priority"
// (priority optional, defaulting to 0). Throws std::invalid_argument on a
// malformed token.
Process parseInlineProcess(const std::string& spec);

// Read a whitespace/column separated process table from a file. Blank lines and
// lines beginning with '#' are ignored. Each data line is
// "id arrival burst [priority]". Throws std::runtime_error on I/O failure and
// std::invalid_argument on a malformed row.
std::vector<Process> loadProcessesFromFile(const std::string& path);

}  // namespace cpusim

#endif  // CPUSIM_PROCESS_HPP
