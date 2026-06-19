#include "scheduler.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace cpusim {

int ScheduleResult::makespan() const {
    if (timeline.empty()) {
        return 0;
    }
    return timeline.back().end - timeline.front().start;
}

namespace {

// Append a one-unit-or-more run of `id` to the timeline, merging with the
// previous slice when it refers to the same process so the raw timeline stays
// compact even before an explicit coalesce pass.
void pushSlice(std::vector<GanttSlice>& timeline, const std::string& id, int start, int end) {
    if (start >= end) {
        return;
    }
    if (!timeline.empty() && timeline.back().id == id && timeline.back().end == start) {
        timeline.back().end = end;
    } else {
        timeline.push_back(GanttSlice{id, start, end});
    }
}

// Stamp the response time the first time a process touches the CPU.
void recordFirstRun(std::unordered_map<std::string, int>& firstRun, const std::string& id, int t) {
    if (firstRun.find(id) == firstRun.end()) {
        firstRun[id] = t;
    }
}

// Fill in derived metrics and aggregate averages from completion/first-run data
// gathered during the simulation. `busyTime` is the total non-idle CPU time and
// `span` is the full timeline length used for the utilization figure.
ScheduleResult finalize(const std::string& name,
                        const std::vector<Process>& procs,
                        std::vector<GanttSlice> timeline,
                        const std::unordered_map<std::string, int>& completion,
                        const std::unordered_map<std::string, int>& firstRun,
                        int busyTime,
                        int span) {
    ScheduleResult res;
    res.algorithm = name;
    res.timeline = std::move(timeline);

    double sumWait = 0.0, sumTurn = 0.0, sumResp = 0.0;
    for (const Process& p : procs) {
        ProcessResult r;
        r.id = p.id;
        r.arrival = p.arrival;
        r.burst = p.burst;
        r.priority = p.priority;
        r.completion = completion.at(p.id);
        r.turnaround = r.completion - p.arrival;
        r.waiting = r.turnaround - p.burst;
        r.response = firstRun.at(p.id) - p.arrival;

        sumWait += r.waiting;
        sumTurn += r.turnaround;
        sumResp += r.response;
        res.processes.push_back(r);
    }

    if (!procs.empty()) {
        const double n = static_cast<double>(procs.size());
        res.avgWaiting = sumWait / n;
        res.avgTurnaround = sumTurn / n;
        res.avgResponse = sumResp / n;
    }
    res.cpuUtilization = (span > 0) ? (100.0 * busyTime / span) : 0.0;
    return res;
}

// Stable ordering helper: by arrival, then by the original input order so that
// equal-arrival ties resolve deterministically.
void sortByArrival(std::vector<Process>& procs) {
    std::stable_sort(procs.begin(), procs.end(), [](const Process& a, const Process& b) {
        return a.arrival < b.arrival;
    });
}

}  // namespace

ScheduleResult scheduleFCFS(std::vector<Process> procs) {
    sortByArrival(procs);

    std::vector<GanttSlice> timeline;
    std::unordered_map<std::string, int> completion, firstRun;
    int now = 0, busy = 0;

    for (const Process& p : procs) {
        if (now < p.arrival) {
            pushSlice(timeline, "idle", now, p.arrival);
            now = p.arrival;
        }
        recordFirstRun(firstRun, p.id, now);
        pushSlice(timeline, p.id, now, now + p.burst);
        now += p.burst;
        busy += p.burst;
        completion[p.id] = now;
    }

    return finalize("FCFS", procs, std::move(timeline), completion, firstRun, busy, now);
}

ScheduleResult scheduleSJF(std::vector<Process> procs) {
    sortByArrival(procs);
    const size_t n = procs.size();

    std::vector<GanttSlice> timeline;
    std::unordered_map<std::string, int> completion, firstRun;
    std::vector<bool> done(n, false);
    int now = 0, busy = 0, finished = 0;

    while (finished < static_cast<int>(n)) {
        int pick = -1;
        for (size_t i = 0; i < n; ++i) {
            if (done[i] || procs[i].arrival > now) {
                continue;
            }
            if (pick == -1 || procs[i].burst < procs[pick].burst) {
                pick = static_cast<int>(i);
            }
        }

        if (pick == -1) {
            // No process has arrived yet; advance to the next arrival, leaving
            // an idle slice in the timeline for the gap.
            int next = std::numeric_limits<int>::max();
            for (size_t i = 0; i < n; ++i) {
                if (!done[i]) {
                    next = std::min(next, procs[i].arrival);
                }
            }
            pushSlice(timeline, "idle", now, next);
            now = next;
            continue;
        }

        const Process& p = procs[pick];
        recordFirstRun(firstRun, p.id, now);
        pushSlice(timeline, p.id, now, now + p.burst);
        now += p.burst;
        busy += p.burst;
        completion[p.id] = now;
        done[pick] = true;
        ++finished;
    }

    return finalize("SJF (non-preemptive)", procs, std::move(timeline), completion, firstRun, busy,
                    now);
}

ScheduleResult scheduleSRTF(std::vector<Process> procs) {
    sortByArrival(procs);
    const size_t n = procs.size();

    std::vector<int> remaining(n);
    for (size_t i = 0; i < n; ++i) {
        remaining[i] = procs[i].burst;
    }

    std::vector<GanttSlice> timeline;
    std::unordered_map<std::string, int> completion, firstRun;
    int now = 0, busy = 0, finished = 0;

    while (finished < static_cast<int>(n)) {
        int pick = -1;
        for (size_t i = 0; i < n; ++i) {
            if (remaining[i] <= 0 || procs[i].arrival > now) {
                continue;
            }
            if (pick == -1 || remaining[i] < remaining[pick]) {
                pick = static_cast<int>(i);
            }
        }

        if (pick == -1) {
            int next = std::numeric_limits<int>::max();
            for (size_t i = 0; i < n; ++i) {
                if (remaining[i] > 0) {
                    next = std::min(next, procs[i].arrival);
                }
            }
            pushSlice(timeline, "idle", now, next);
            now = next;
            continue;
        }

        recordFirstRun(firstRun, procs[pick].id, now);
        // Run for a single tick so a newly arriving shorter job can preempt.
        pushSlice(timeline, procs[pick].id, now, now + 1);
        --remaining[pick];
        ++now;
        ++busy;
        if (remaining[pick] == 0) {
            completion[procs[pick].id] = now;
            ++finished;
        }
    }

    coalesceTimeline(timeline);
    return finalize("SRTF (preemptive)", procs, std::move(timeline), completion, firstRun, busy,
                    now);
}

ScheduleResult scheduleRoundRobin(std::vector<Process> procs, int quantum) {
    if (quantum <= 0) {
        throw std::invalid_argument("round-robin quantum must be a positive integer");
    }
    sortByArrival(procs);
    const size_t n = procs.size();

    std::vector<int> remaining(n);
    for (size_t i = 0; i < n; ++i) {
        remaining[i] = procs[i].burst;
    }

    std::vector<GanttSlice> timeline;
    std::unordered_map<std::string, int> completion, firstRun;
    std::vector<int> queue;       // indices of ready processes, FIFO
    std::vector<bool> enqueued(n, false);
    int now = 0, busy = 0, finished = 0, scanned = 0;

    // Helper to admit every process that has arrived by time `t` in arrival
    // order, preserving the FIFO discipline that defines round robin.
    auto admit = [&](int t) {
        while (scanned < static_cast<int>(n) && procs[scanned].arrival <= t) {
            if (!enqueued[scanned]) {
                queue.push_back(scanned);
                enqueued[scanned] = true;
            }
            ++scanned;
        }
    };

    admit(now);

    while (finished < static_cast<int>(n)) {
        if (queue.empty()) {
            // Jump to the next arrival, leaving an idle slice for the gap.
            int next = std::numeric_limits<int>::max();
            for (size_t i = 0; i < n; ++i) {
                if (remaining[i] > 0 && !enqueued[i]) {
                    next = std::min(next, procs[i].arrival);
                }
            }
            if (next == std::numeric_limits<int>::max()) {
                break;
            }
            pushSlice(timeline, "idle", now, next);
            now = next;
            admit(now);
            continue;
        }

        int idx = queue.front();
        queue.erase(queue.begin());

        int run = std::min(quantum, remaining[idx]);
        recordFirstRun(firstRun, procs[idx].id, now);
        pushSlice(timeline, procs[idx].id, now, now + run);
        now += run;
        busy += run;
        remaining[idx] -= run;

        // Admit processes that arrived while this slice was running before the
        // current process is re-queued, matching standard textbook behaviour.
        admit(now);

        if (remaining[idx] == 0) {
            completion[procs[idx].id] = now;
            ++finished;
        } else {
            queue.push_back(idx);
        }
    }

    return finalize("Round Robin (q=" + std::to_string(quantum) + ")", procs, std::move(timeline),
                    completion, firstRun, busy, now);
}

ScheduleResult schedulePriority(std::vector<Process> procs, bool preemptive) {
    sortByArrival(procs);
    const size_t n = procs.size();

    std::vector<int> remaining(n);
    for (size_t i = 0; i < n; ++i) {
        remaining[i] = procs[i].burst;
    }

    std::vector<GanttSlice> timeline;
    std::unordered_map<std::string, int> completion, firstRun;
    std::vector<bool> done(n, false);
    int now = 0, busy = 0, finished = 0;

    auto bestReady = [&]() -> int {
        int pick = -1;
        for (size_t i = 0; i < n; ++i) {
            if (done[i] || procs[i].arrival > now) {
                continue;
            }
            // Lower priority value wins; arrival breaks ties (procs is arrival
            // sorted, so the earlier index is the earlier arrival).
            if (pick == -1 || procs[i].priority < procs[pick].priority) {
                pick = static_cast<int>(i);
            }
        }
        return pick;
    };

    while (finished < static_cast<int>(n)) {
        int pick = bestReady();
        if (pick == -1) {
            int next = std::numeric_limits<int>::max();
            for (size_t i = 0; i < n; ++i) {
                if (!done[i]) {
                    next = std::min(next, procs[i].arrival);
                }
            }
            pushSlice(timeline, "idle", now, next);
            now = next;
            continue;
        }

        if (preemptive) {
            recordFirstRun(firstRun, procs[pick].id, now);
            pushSlice(timeline, procs[pick].id, now, now + 1);
            --remaining[pick];
            ++now;
            ++busy;
            if (remaining[pick] == 0) {
                completion[procs[pick].id] = now;
                done[pick] = true;
                ++finished;
            }
        } else {
            recordFirstRun(firstRun, procs[pick].id, now);
            pushSlice(timeline, procs[pick].id, now, now + procs[pick].burst);
            now += procs[pick].burst;
            busy += procs[pick].burst;
            completion[procs[pick].id] = now;
            done[pick] = true;
            ++finished;
        }
    }

    coalesceTimeline(timeline);
    return finalize(preemptive ? "Priority (preemptive)" : "Priority (non-preemptive)", procs,
                    std::move(timeline), completion, firstRun, busy, now);
}

void coalesceTimeline(std::vector<GanttSlice>& timeline) {
    if (timeline.empty()) {
        return;
    }
    std::vector<GanttSlice> merged;
    merged.push_back(timeline.front());
    for (size_t i = 1; i < timeline.size(); ++i) {
        GanttSlice& back = merged.back();
        if (timeline[i].id == back.id && timeline[i].start == back.end) {
            back.end = timeline[i].end;
        } else {
            merged.push_back(timeline[i]);
        }
    }
    timeline.swap(merged);
}

}  // namespace cpusim
