# Build and run the host-side unit tests.
#   make        # build + run
#   make clean
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O0 -g
SRC      := main.cpp tests_layout.cpp tests_bmp.cpp tests_status.cpp \
            ../src/layout.cpp ../src/bmp.cpp ../src/status.cpp
BIN      := run_tests

.PHONY: all clean
all: $(BIN)
	./$(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

clean:
	rm -f $(BIN)
