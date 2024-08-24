# Directories
SRC_DIR = src
OBJ_DIR = obj
# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Flags
CC = gcc
CFLAGS = -I./include
LDFLAGS =  -lev

# Executable
TARGET = dns-proxy

# Default target
all: $(TARGET)

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Linking
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Include the dependency files generated
-include $(OBJS:.o=.d)

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(OBJS:.o=.d)

# Phony targets
.PHONY: all clean
