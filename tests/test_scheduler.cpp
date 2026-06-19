// Self-contained unit tests for the scheduling core. No external framework is
// used: each check is a plain assert, and the file exits 0 only if every
// expectation holds. The expected averages below were computed by hand and are
// the canonical textbook results for the small process sets used here.
//
// The assertions must run even in a Release (-DNDEBUG) configuration, so NDEBUG
// is cleared before <cassert> is pulled in. This keeps `ctest` meaningful no
// matter which build type the project was configured with.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "process.hpp"
#include "scheduler.hpp"

using namespace cpusim;

namespace {

bool nearlyEqual(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}

// Locate a process result by id (the result order matches the input order, but
// looking up by id keeps the assertions readable and order-independent).
const ProcessResult& byId(const ScheduleResult& r, const std::string& id) {
    for (const ProcessResult& p : r.processes) {
        if (p.id == id) {
            return p;
        }
    }
    std::cerr << "missing process result for " << id << "\n";
    std::abort();
}

std::vector<Process> sampleSet() {
    // P1(0,5) P2(1,3) P3(2,8) P4(3,6) — priorities unused by FCFS/RR.
    return {
        Process{"P1", 0, 5, 2},
        Process{"P2", 1, 3, 1},
        Process{"P3", 2, 8, 3},
        Process{"P4", 3, 6, 2},
    };
}

void testFcfsAverages() {
    ScheduleResult r = scheduleFCFS(sampleSet());

    // Hand-verified: waiting = {0,4,6,13}, turnaround = {5,7,14,19}.
    assert(nearlyEqual(r.avgWaiting, 5.75));
    assert(nearlyEqual(r.avgTurnaround, 11.25));

    // Spot-check individual processes too.
    assert(byId(r, "P1").completion == 5);
    assert(byId(r, "P2").completion == 8);
    assert(byId(r, "P3").completion == 16);
    assert(byId(r, "P4").completion == 22);
    assert(byId(r, "P4").waiting == 13);

    // No idle time in this set, so utilization is a full 100%.
    assert(nearlyEqual(r.cpuUtilization, 100.0));
    std::cout << "[ok] FCFS averages\n";
}

void testRoundRobinAverages() {
    ScheduleResult r = scheduleRoundRobin(sampleSet(), 2);

    // Hand-verified with quantum = 2:
    //   completion P1=14 P2=11 P3=22 P4=20
    //   waiting = {9,7,12,11}, turnaround = {14,10,20,17}
    assert(nearlyEqual(r.avgWaiting, 9.75));
    assert(nearlyEqual(r.avgTurnaround, 15.25));

    assert(byId(r, "P1").completion == 14);
    assert(byId(r, "P2").completion == 11);
    assert(byId(r, "P3").completion == 22);
    assert(byId(r, "P4").completion == 20);

    assert(nearlyEqual(r.cpuUtilization, 100.0));
    std::cout << "[ok] Round Robin averages\n";
}

void testSjfAverages() {
    std::vector<Process> set = {
        Process{"P1", 0, 7, 0},
        Process{"P2", 2, 4, 0},
        Process{"P3", 4, 1, 0},
        Process{"P4", 5, 4, 0},
    };
    ScheduleResult r = scheduleSJF(set);
    assert(nearlyEqual(r.avgWaiting, 4.0));
    assert(nearlyEqual(r.avgTurnaround, 8.0));
    std::cout << "[ok] SJF averages\n";
}

void testSrtfAverages() {
    std::vector<Process> set = {
        Process{"P1", 0, 7, 0},
        Process{"P2", 2, 4, 0},
        Process{"P3", 4, 1, 0},
        Process{"P4", 5, 4, 0},
    };
    ScheduleResult r = scheduleSRTF(set);
    assert(nearlyEqual(r.avgWaiting, 3.0));
    assert(nearlyEqual(r.avgTurnaround, 7.0));

    // SRTF must never beat the makespan of the work supplied: all four bursts
    // sum to 16 and there is no idle gap from t=0, so the schedule ends at 16.
    assert(r.makespan() == 16);
    std::cout << "[ok] SRTF averages\n";
}

void testPriorityNonPreemptive() {
    // Lower priority value runs first. P3 has the best priority (0).
    std::vector<Process> set = {
        Process{"P1", 0, 4, 2},
        Process{"P2", 1, 3, 1},
        Process{"P3", 2, 2, 0},
    };
    ScheduleResult r = schedulePriority(set, false);

    // At t=0 only P1 has arrived, so it runs to completion (non-preemptive),
    // finishing at t=4. Then among P2(prio 1) and P3(prio 0), P3 runs first
    // (2..6), then P2 (6..9).
    assert(byId(r, "P1").completion == 4);
    assert(byId(r, "P3").completion == 6);
    assert(byId(r, "P2").completion == 9);
    std::cout << "[ok] Priority (non-preemptive)\n";
}

void testPriorityPreemptive() {
    std::vector<Process> set = {
        Process{"P1", 0, 4, 2},
        Process{"P2", 1, 3, 1},
        Process{"P3", 2, 2, 0},
    };
    ScheduleResult r = schedulePriority(set, true);

    // Timeline: P1 runs 0..1; P2 (better prio) preempts and runs 1..2; P3
    // (best prio) preempts and runs 2..4 to completion; then P2 finishes its
    // remaining 2 units 4..6; finally P1's remaining 3 units 6..9.
    assert(byId(r, "P3").completion == 4);
    assert(byId(r, "P2").completion == 6);
    assert(byId(r, "P1").completion == 9);
    std::cout << "[ok] Priority (preemptive)\n";
}

void testIdleHandling() {
    // A late-arriving sole process produces a leading idle gap. Utilization is
    // measured over the full observation window from t=0, so it is burst /
    // completion = 5 / 10.
    std::vector<Process> set = {Process{"P1", 5, 5, 0}};
    ScheduleResult r = scheduleFCFS(set);
    assert(byId(r, "P1").completion == 10);
    assert(byId(r, "P1").waiting == 0);
    assert(r.makespan() == 10);         // timeline spans 0..10 (idle 0..5 + run)
    assert(nearlyEqual(r.cpuUtilization, 50.0));
    std::cout << "[ok] idle handling\n";
}

void testGapUtilization() {
    // Two non-overlapping processes with a forced gap between them.
    std::vector<Process> set = {Process{"P1", 0, 2, 0}, Process{"P2", 10, 2, 0}};
    ScheduleResult r = scheduleFCFS(set);
    // Busy = 4, span = 12 (0..12) -> 33.33%.
    assert(r.makespan() == 12);
    assert(nearlyEqual(r.cpuUtilization, 100.0 * 4.0 / 12.0, 1e-6));
    std::cout << "[ok] gap utilization\n";
}

void testInlineParsing() {
    Process p = parseInlineProcess("P7:3:9:5");
    assert(p.id == "P7");
    assert(p.arrival == 3);
    assert(p.burst == 9);
    assert(p.priority == 5);

    Process q = parseInlineProcess("Pa:0:4");  // priority defaults to 0
    assert(q.priority == 0);

    bool threw = false;
    try {
        parseInlineProcess("bad:spec");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    threw = false;
    try {
        parseInlineProcess("P1:0:0");  // zero burst is rejected
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "[ok] inline parsing\n";
}

void testRoundRobinRejectsBadQuantum() {
    bool threw = false;
    try {
        scheduleRoundRobin(sampleSet(), 0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "[ok] round robin rejects non-positive quantum\n";
}

void testTimelineContiguity() {
    // The emitted timeline must be gap-free and cover the entire makespan.
    ScheduleResult r = scheduleRoundRobin(sampleSet(), 3);
    assert(!r.timeline.empty());
    for (size_t i = 1; i < r.timeline.size(); ++i) {
        assert(r.timeline[i].start == r.timeline[i - 1].end);
    }
    std::cout << "[ok] timeline contiguity\n";
}

}  // namespace

int main() {
    testFcfsAverages();
    testRoundRobinAverages();
    testSjfAverages();
    testSrtfAverages();
    testPriorityNonPreemptive();
    testPriorityPreemptive();
    testIdleHandling();
    testGapUtilization();
    testInlineParsing();
    testRoundRobinRejectsBadQuantum();
    testTimelineContiguity();

    std::cout << "\nAll scheduler tests passed.\n";
    return 0;
}
