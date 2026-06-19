#ifndef CPUSIM_REPORT_HPP
#define CPUSIM_REPORT_HPP

#include <ostream>
#include <vector>

#include "scheduler.hpp"

namespace cpusim {

// Render an ASCII Gantt chart for a single schedule, e.g.
//
//   | P1 | P2 |  idle  | P3 |
//   0    4    7        9    14
//
// The chart scales each slice's width to its duration so the relative timing is
// visually apparent.
void renderGantt(std::ostream& os, const ScheduleResult& result);

// Print the per-process metric table and aggregate averages for one schedule.
void renderMetrics(std::ostream& os, const ScheduleResult& result);

// Print a full report (header, Gantt chart, metrics) for one schedule.
void renderReport(std::ostream& os, const ScheduleResult& result);

// Print a side-by-side comparison of average waiting/turnaround/response times
// and CPU utilization across several schedules (used by `--algo all`).
void renderComparison(std::ostream& os, const std::vector<ScheduleResult>& results);

}  // namespace cpusim

#endif  // CPUSIM_REPORT_HPP
