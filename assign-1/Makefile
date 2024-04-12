# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++17 -Wall

# Source files
SRCS = main.cpp nfa.cpp re.cpp

# Header files
HDRS = nfa.hpp re.hpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
EXEC = lex

# Default target
all: $(EXEC)

# Linking step
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# Object files dependencies
main.o: main.cpp $(HDRS)
	$(CC) $(CFLAGS) -c main.cpp -o main.o

nfa.o: nfa.cpp nfa.hpp re.hpp
	$(CC) $(CFLAGS) -c nfa.cpp -o nfa.o

re.o: re.cpp re.hpp nfa.hpp
	$(CC) $(CFLAGS) -c re.cpp -o re.o

# Run the executable
run: $(EXEC)
	./$(EXEC)

# Clean up
clean:
	rm -f $(EXEC) $(OBJS)
