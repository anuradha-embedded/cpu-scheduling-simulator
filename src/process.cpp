#include "process.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace cpusim {

namespace {

// Split a string on a single delimiter, preserving empty fields so callers can
// detect a missing component (e.g. "P1::5").
std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string field;
    std::istringstream iss(s);
    while (std::getline(iss, field, delim)) {
        out.push_back(field);
    }
    // getline does not emit a trailing empty field for a terminal delimiter;
    // handle that explicitly so "P1:0:5:" parses as four fields.
    if (!s.empty() && s.back() == delim) {
        out.emplace_back();
    }
    return out;
}

int parseInt(const std::string& field, const std::string& context) {
    if (field.empty()) {
        throw std::invalid_argument("empty numeric field in " + context);
    }
    try {
        size_t consumed = 0;
        int value = std::stoi(field, &consumed);
        if (consumed != field.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return value;
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid integer '" + field + "' in " + context);
    }
}

}  // namespace

Process parseInlineProcess(const std::string& spec) {
    std::vector<std::string> parts = split(spec, ':');
    if (parts.size() < 3 || parts.size() > 4) {
        throw std::invalid_argument(
            "process spec must be id:arrival:burst[:priority], got '" + spec + "'");
    }

    Process p;
    p.id = parts[0];
    if (p.id.empty()) {
        throw std::invalid_argument("process id must not be empty in '" + spec + "'");
    }
    p.arrival = parseInt(parts[1], "arrival of '" + spec + "'");
    p.burst = parseInt(parts[2], "burst of '" + spec + "'");
    p.priority = (parts.size() == 4) ? parseInt(parts[3], "priority of '" + spec + "'") : 0;

    if (p.arrival < 0) {
        throw std::invalid_argument("arrival time must be non-negative for '" + p.id + "'");
    }
    if (p.burst <= 0) {
        throw std::invalid_argument("burst time must be positive for '" + p.id + "'");
    }
    return p;
}

std::vector<Process> loadProcessesFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open process file: " + path);
    }

    std::vector<Process> procs;
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;

        // Strip an inline comment and surrounding whitespace.
        size_t hash = line.find('#');
        if (hash != std::string::npos) {
            line.erase(hash);
        }
        std::istringstream iss(line);

        Process p;
        std::string priorityTok;
        if (!(iss >> p.id >> p.arrival >> p.burst)) {
            // A line that yields no tokens at all is simply blank; skip it.
            if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }
            throw std::invalid_argument(
                path + ":" + std::to_string(lineNo) + ": expected 'id arrival burst [priority]'");
        }
        if (iss >> priorityTok) {
            p.priority = parseInt(priorityTok, path + ":" + std::to_string(lineNo));
        }

        if (p.arrival < 0) {
            throw std::invalid_argument(
                path + ":" + std::to_string(lineNo) + ": arrival must be non-negative");
        }
        if (p.burst <= 0) {
            throw std::invalid_argument(
                path + ":" + std::to_string(lineNo) + ": burst must be positive");
        }
        procs.push_back(std::move(p));
    }

    return procs;
}

}  // namespace cpusim
