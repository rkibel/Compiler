# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++17 -Wall

# Source files
SRCS_PARSER = main.cpp
SRCS_LEXER = assign-1.cpp
SRCS_PROGRAM = program.cpp

# Intermediate files
LEX_OUTPUT = lexer-output.txt

# Executable names
EXEC_PARSER = parse
EXEC_LEXER = lexer

# Default target
all: $(EXEC_PARSER) $(EXEC_LEXER)

# Build the lexer executable, using "make lexer"
$(EXEC_LEXER): $(SRCS_LEXER)
	$(CC) $(CFLAGS) $(SRCS_LEXER) -o $(EXEC_LEXER)

# Build the parser executable
$(EXEC_PARSER): $(SRCS_PARSER) $(SRCS_PROGRAM) 
	$(CC) $(CFLAGS) $(SRCS_PARSER) -o $(EXEC_PARSER)

# IMPORTANT! execute "make run {original unlexed file name here} and this will lex, then run assign-2
# If you just want to run assign-2 on a lexed input, do ./assign-2 {name of lexed file here}
run: $(EXEC_PARSER)
	./$(EXEC_LEXER) $(filter-out $@,$(MAKECMDGOALS)) $(LEX_OUTPUT)
	./$(EXEC_PARSER) $(LEX_OUTPUT)

# Clean up
clean:
	rm -f $(EXEC_PARSER) $(EXEC_LEXER) $(LEX_OUTPUT)