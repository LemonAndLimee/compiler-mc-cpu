# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -g -std=c++17 -DOS_X

# Target executable
TARGET_DIR=Bin
TARGET = $(TARGET_DIR)/Compiler

# Source files
SRCS=$(wildcard *.cpp)
# Object files
OBJS_NAMES = $(SRCS:.cpp=.o)
OBJS_DIR = $(TARGET_DIR)/Objs
OBJS = $(patsubst %,$(OBJS_DIR)/%,$(OBJS_NAMES))

# Default rule to build the executable
all: clean directory $(TARGET)

# Rule to make objects directory
directory:
	@mkdir -p $(TARGET_DIR)
	@mkdir -p $(OBJS_DIR)

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile .cpp files into .o files
$(OBJS_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to run the executable
run: $(TARGET)
	./$(TARGET)

# Clean rule to remove generated files
clean:
	-rm $(TARGET)
	-rm $(OBJS)