#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "process.hpp"
#include "report.hpp"
#include "scheduler.hpp"

namespace {

using cpusim::Process;
using cpusim::ScheduleResult;

const char* kUsage =
    "cpusim - CPU scheduling simulator\n"
    "\n"
    "Usage:\n"
    "  cpusim --algo <name> [options]\n"
    "\n"
    "Algorithms (--algo):\n"
    "  fcfs        First-Come, First-Served\n"
    "  sjf         Shortest Job First (non-preemptive)\n"
    "  srtf        Shortest Remaining Time First (preemptive)\n"
    "  rr          Round Robin (requires --quantum)\n"
    "  priority    Priority, non-preemptive (default)\n"
    "  priority-p  Priority, preemptive\n"
    "  all         Run every algorithm and print a comparison table\n"
    "\n"
    "Input (combine freely; at least one required):\n"
    "  --input <file>          load a process table from a file\n"
    "  --proc id:arr:burst[:prio]   add one inline process (repeatable)\n"
    "\n"
    "Options:\n"
    "  --quantum <N>           time quantum for round robin (default 2)\n"
    "  -h, --help              show this help\n"
    "\n"
    "Examples:\n"
    "  cpusim --algo fcfs --proc P1:0:5 --proc P2:1:3\n"
    "  cpusim --algo rr --quantum 3 --input examples/procs.txt\n"
    "  cpusim --algo all --input examples/procs.txt\n";

struct Options {
    std::string algo;
    std::string inputFile;
    std::vector<std::string> inlineSpecs;
    int quantum = 2;
    bool quantumSet = false;
    bool help = false;
};

// Pull the value that follows a flag, erroring if it is missing.
std::string requireValue(int argc, char** argv, int& i, const std::string& flag) {
    if (i + 1 >= argc) {
        throw std::invalid_argument("missing value for " + flag);
    }
    return argv[++i];
}

Options parseArgs(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            opt.help = true;
        } else if (arg == "--algo") {
            opt.algo = requireValue(argc, argv, i, arg);
        } else if (arg == "--input") {
            opt.inputFile = requireValue(argc, argv, i, arg);
        } else if (arg == "--proc") {
            opt.inlineSpecs.push_back(requireValue(argc, argv, i, arg));
        } else if (arg == "--quantum") {
            std::string v = requireValue(argc, argv, i, arg);
            try {
                opt.quantum = std::stoi(v);
            } catch (const std::exception&) {
                throw std::invalid_argument("invalid --quantum value: " + v);
            }
            opt.quantumSet = true;
        } else {
            throw std::invalid_argument("unknown argument: " + arg);
        }
    }
    return opt;
}

std::vector<Process> gatherProcesses(const Options& opt) {
    std::vector<Process> procs;
    if (!opt.inputFile.empty()) {
        std::vector<Process> fromFile = cpusim::loadProcessesFromFile(opt.inputFile);
        procs.insert(procs.end(), fromFile.begin(), fromFile.end());
    }
    for (const std::string& spec : opt.inlineSpecs) {
        procs.push_back(cpusim::parseInlineProcess(spec));
    }
    return procs;
}

ScheduleResult run(const std::string& algo, const std::vector<Process>& procs, int quantum) {
    if (algo == "fcfs") {
        return cpusim::scheduleFCFS(procs);
    }
    if (algo == "sjf") {
        return cpusim::scheduleSJF(procs);
    }
    if (algo == "srtf") {
        return cpusim::scheduleSRTF(procs);
    }
    if (algo == "rr") {
        return cpusim::scheduleRoundRobin(procs, quantum);
    }
    if (algo == "priority" || algo == "priority-np") {
        return cpusim::schedulePriority(procs, false);
    }
    if (algo == "priority-p") {
        return cpusim::schedulePriority(procs, true);
    }
    throw std::invalid_argument("unknown algorithm: " + algo);
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    try {
        opt = parseArgs(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n\n" << kUsage;
        return 2;
    }

    if (opt.help || argc == 1) {
        std::cout << kUsage;
        return opt.help ? 0 : 2;
    }

    if (opt.algo.empty()) {
        std::cerr << "error: --algo is required\n\n" << kUsage;
        return 2;
    }

    std::vector<Process> procs;
    try {
        procs = gatherProcesses(opt);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }

    if (procs.empty()) {
        std::cerr << "error: no processes supplied; use --input and/or --proc\n";
        return 2;
    }

    try {
        if (opt.algo == "all") {
            std::vector<ScheduleResult> results;
            results.push_back(cpusim::scheduleFCFS(procs));
            results.push_back(cpusim::scheduleSJF(procs));
            results.push_back(cpusim::scheduleSRTF(procs));
            results.push_back(cpusim::scheduleRoundRobin(procs, opt.quantum));
            results.push_back(cpusim::schedulePriority(procs, false));
            results.push_back(cpusim::schedulePriority(procs, true));

            for (const ScheduleResult& r : results) {
                cpusim::renderReport(std::cout, r);
            }
            cpusim::renderComparison(std::cout, results);
        } else {
            ScheduleResult result = run(opt.algo, procs, opt.quantum);
            cpusim::renderReport(std::cout, result);
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
