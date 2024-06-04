# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++17 -Wall 

# Source files
SRCS_MAIN = main.cpp
SRCS_LIR = lir.cpp

# Executable names
EXEC_OPT = opt

# Default target
all: $(EXEC_OPT)

# Build the executable
$(EXEC_OPT): $(SRCS_MAIN) $(SRCS_LIR)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: run
run:
	ref/lex $(filter-out $@,$(MAKECMDGOALS)) > test/lexer_output.txt;
	ref/parse -json test/lexer_output.txt > test/parse_output.txt; 
	ref/lower -json test/parse_output.txt > test/lower_output.txt;
	./opt test/lower_output.txt;

# Can call ref either with the json or the hr flag, will change how the lowering looks
# make ref FLAG=json test-in.txt
.PHONY: ref
ref:
	ref/lex $(filter-out $@,$(MAKECMDGOALS)) > ref/lexer_output_ref.txt;
	ref/parse -json ref/lexer_output_ref.txt > ref/parse_output_ref.txt; 
	ref/lower -$(FLAG) ref/parse_output_ref.txt > ref/lower_output_ref.txt;
	ref/opt ref/lower_output_ref.txt;

# Clean up
clean:
	rm -f $(EXEC_OPT)