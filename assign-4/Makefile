# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++17 -Wall 

# Source files
SRCS_MAIN = main.cpp
SRCS_LIR = lir.cpp

# Executable names
EXEC_LIR = lir

# Default target
all: $(EXEC_LIR)

# Build the executable
$(EXEC_LIR): $(SRCS_MAIN) $(SRCS_LIR)
	$(CC) $(CFLAGS) -o $@ $^

# Clean up
clean:
	rm -f $(EXEC_LIR)