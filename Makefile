# Cmake-free build for cpusim and its tests.
#
#   make            build the cpusim binary
#   make test       build and run the unit tests
#   make clean      remove build artifacts
#
# Override the compiler with e.g. `make CXX=g++`.

CXX      ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CPPFLAGS += -Iinclude

BUILDDIR := build
BIN      := $(BUILDDIR)/cpusim
TESTBIN  := $(BUILDDIR)/test_scheduler

CORE_SRC := src/process.cpp src/scheduler.cpp src/report.cpp
CORE_OBJ := $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(CORE_SRC))

.PHONY: all test clean

all: $(BIN)

# Order-only prerequisite: ensure the output directory exists before any
# compile, without treating its timestamp as a dependency.
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: src/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILDDIR)/test_scheduler.o: tests/test_scheduler.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BIN): $(CORE_OBJ) $(BUILDDIR)/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TESTBIN): $(CORE_OBJ) $(BUILDDIR)/test_scheduler.o
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(TESTBIN)
	./$(TESTBIN)

clean:
	rm -rf $(BUILDDIR)
