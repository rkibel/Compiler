# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++17 -Wall

# Source files
SRCS = main.cpp

# Executable name
EXEC = lex

# Default target
all: $(EXEC)

# Linking step
$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(EXEC)

run: $(EXEC)
	./$(EXEC)

# Clean up
clean:
	rm -f $(EXEC)
