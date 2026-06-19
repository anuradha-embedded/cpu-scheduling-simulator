#include "report.hpp"

#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

namespace cpusim {

namespace {

// Render one cell of the Gantt bar. Each slice is drawn proportionally to its
// duration, with a sensible minimum so a label always fits.
std::string centeredLabel(const std::string& label, int width) {
    if (static_cast<int>(label.size()) >= width) {
        return label;
    }
    int total = width - static_cast<int>(label.size());
    int left = total / 2;
    int right = total - left;
    return std::string(left, ' ') + label + std::string(right, ' ');
}

}  // namespace

void renderGantt(std::ostream& os, const ScheduleResult& result) {
    if (result.timeline.empty()) {
        os << "(empty schedule)\n";
        return;
    }

    // Each slice gets a cell at least wide enough for its label plus padding,
    // and otherwise scaled to its duration so relative timing reads clearly.
    // We track per-slice cell widths so the boundary tick row can be aligned
    // exactly beneath each '+' separator.
    std::string top = "+";
    std::string mid = "|";
    std::vector<int> cells;
    cells.reserve(result.timeline.size());

    for (const GanttSlice& slice : result.timeline) {
        int duration = slice.end - slice.start;
        int cell = std::max<int>(static_cast<int>(slice.id.size()) + 2, duration + 2);
        cells.push_back(cell);
        mid += centeredLabel(slice.id, cell) + "|";
        top += std::string(static_cast<size_t>(cell), '-') + "+";
    }

    os << top << "\n";
    os << mid << "\n";

    // Boundary tick row: align each end-time label under the '+' that closes
    // the corresponding cell. Column 0 holds the very first start time.
    std::string ticks = std::to_string(result.timeline.front().start);
    size_t col = ticks.size();
    size_t pos = 1;  // first cell begins just after the leading '+'
    for (size_t i = 0; i < result.timeline.size(); ++i) {
        pos += static_cast<size_t>(cells[i]) + 1;  // cell width plus trailing '+'
        std::string label = std::to_string(result.timeline[i].end);
        size_t target = pos - 1;                    // column of the closing '+'
        if (col < target) {
            ticks += std::string(target - col, ' ');
            col = target;
        }
        ticks += label;
        col += label.size();
    }
    os << ticks << "\n";
}

void renderMetrics(std::ostream& os, const ScheduleResult& result) {
    os << std::left;
    os << std::setw(6) << "PID" << std::right << std::setw(9) << "Arrival" << std::setw(7)
       << "Burst" << std::setw(9) << "Priority" << std::setw(11) << "Completion" << std::setw(11)
       << "Turnaround" << std::setw(9) << "Waiting" << std::setw(10) << "Response" << "\n";
    os << std::string(72, '-') << "\n";

    for (const ProcessResult& r : result.processes) {
        os << std::left << std::setw(6) << r.id << std::right << std::setw(9) << r.arrival
           << std::setw(7) << r.burst << std::setw(9) << r.priority << std::setw(11) << r.completion
           << std::setw(11) << r.turnaround << std::setw(9) << r.waiting << std::setw(10)
           << r.response << "\n";
    }

    os << std::string(72, '-') << "\n";
    os << std::fixed << std::setprecision(2);
    os << "Average waiting time   : " << result.avgWaiting << "\n";
    os << "Average turnaround time: " << result.avgTurnaround << "\n";
    os << "Average response time  : " << result.avgResponse << "\n";
    os << "CPU utilization        : " << result.cpuUtilization << "%\n";
    os.unsetf(std::ios::floatfield);
}

void renderReport(std::ostream& os, const ScheduleResult& result) {
    os << "=== " << result.algorithm << " ===\n\n";
    os << "Gantt chart:\n";
    renderGantt(os, result);
    os << "\n";
    renderMetrics(os, result);
    os << "\n";
}

void renderComparison(std::ostream& os, const std::vector<ScheduleResult>& results) {
    if (results.empty()) {
        return;
    }
    os << "=== Comparison ===\n\n";
    os << std::left << std::setw(28) << "Algorithm" << std::right << std::setw(12) << "Avg Wait"
       << std::setw(14) << "Avg Turn" << std::setw(14) << "Avg Resp" << std::setw(10) << "CPU %"
       << "\n";
    os << std::string(78, '-') << "\n";
    os << std::fixed << std::setprecision(2);
    for (const ScheduleResult& r : results) {
        os << std::left << std::setw(28) << r.algorithm << std::right << std::setw(12)
           << r.avgWaiting << std::setw(14) << r.avgTurnaround << std::setw(14) << r.avgResponse
           << std::setw(10) << r.cpuUtilization << "\n";
    }
    os.unsetf(std::ios::floatfield);
    os << "\n";
}

}  // namespace cpusim
