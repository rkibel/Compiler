# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++17 -Wall -g

# Source files
SRC = main.cpp
SRC_AST = ast.cpp

# Intermediate files
INTER_LEXER = lexer-output.txt

# Executable names
EXEC_LEXER = lexer
EXEC_LOWER = lower
EXEC_PARSER = parse

# Default target
all: $(EXEC_LOWER)

# Build the lowering executable
$(EXEC_LOWER): $(SRC) $(SRC_AST)
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC_LOWER)

# Build the parser executable
$(EXEC_PARSER): $(SRC) $(SRC_AST) 
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC_PARSER)

# run raw code
# ex: make run testcode.txt
run: $(EXEC_LOWER)
	./$(EXEC_LEXER) $(filter-out $@,$(MAKECMDGOALS))  > $(INTER_LEXER);
	./$(EXEC_LOWER) temp-arg.txt $(INTER_LEXER)


#Ensure that you are using professor's updated versions of the executables
# ex: make run testcode.txt
ref:
	./$(EXEC_LEXER) $(filter-out $@,$(MAKECMDGOALS)) > lexer-output-ref.txt;
	./parse_ref -json lexer-output-ref.txt > parse-output-ref.txt;
	./lower_ref parse-output-ref.txt

# Clean up
clean:
	rm -f $(EXEC_LOWER) $(INTER_LEXER) lexer-output-ref.txt parse-output.ref.txt lexer-output.txt
