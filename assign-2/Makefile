# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++17 -Wall

# Source files
SRCS_PARSER = main.cpp
SRCS_LEXER = assign-1.cpp

# Executable names
EXEC_PARSER = parse
EXEC_LEXER = lexer

# Default target
all: $(EXEC_PARSER)

# Build the lexer executable, using "make lexer"
$(EXEC_LEXER): $(SRCS_LEXER)
	$(CC) $(CFLAGS) $(SRCS_LEXER) -o $(EXEC_LEXER)

# Build the parser executable
$(EXEC_PARSER): $(SRCS_PARSER)
	$(CC) $(CFLAGS) $(SRCS_PARSER) -o $(EXEC_PARSER)

# IMPORTANT! execute "make run {original unlexed file name here} and this will lex, then run assign-2
# If you just want to run assign-2 on a lexed input, do ./assign-2 {name of lexed file here}
run: $(EXEC_PARSER)
	./lexer $(filter-out $@,$(MAKECMDGOALS)) lexer-output.txt
	./$(EXEC_PARSER) lexer-output.txt

# Clean up
clean:
	rm -f $(EXEC_PARSER) $(EXEC_LEXER)