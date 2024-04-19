# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++17 -Wall

# Source files
SRCS = main.cpp

# Executable name
EXEC = assign-2

# Default target
all: $(EXEC)

# Linking step
$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(EXEC)

# IMPORTANT! execute "make run {original unlexed file name here} and this will lex, then run assign-2
# If you just want to run assign-2 on a lexed input, do ./assign-2 {name of lexed file here}
run: $(EXEC)
	./lexer $(filter-out $@,$(MAKECMDGOALS)) lexer-output.txt
	./$(EXEC) lexer-output.txt

# Clean up
clean:
	rm -f $(EXEC)
