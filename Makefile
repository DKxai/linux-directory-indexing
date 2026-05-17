# ============================================================
# Makefile - Directory Lookup Performance Benchmark
# Week 6: HTree (+ Linear Search + Hash Table + B-Tree)
# ============================================================
# Usage:
#   make            - Build the benchmark program
#   make clean      - Remove build artifacts
#   make run        - Build and run full benchmark
#   make demo       - Build and run interactive demo
#   make valgrind   - Run with valgrind to check memory leaks
# ============================================================

CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -Iinclude
LDFLAGS  = -lm

# Directories
SRC_DIR  = src
INC_DIR  = include
OBJ_DIR  = obj
BIN_DIR  = .

# Source files (Tuan 6: Linear Search + Hash Table + B-Tree + HTree)
SRCS     = $(SRC_DIR)/main.c \
           $(SRC_DIR)/dir_entry.c \
           $(SRC_DIR)/linear_search.c \
           $(SRC_DIR)/hash_table.c \
           $(SRC_DIR)/btree.c \
           $(SRC_DIR)/htree.c \
           $(SRC_DIR)/benchmark.c

# Object files
OBJS     = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Target
TARGET   = $(BIN_DIR)/benchmark

# ============================================================
# Build rules
# ============================================================

.PHONY: all clean run demo valgrind help

all: $(TARGET)
	@echo ""
	@echo "  ✓ Build successful!"
	@echo "  Run: ./benchmark          (interactive menu)"
	@echo "  Run: ./benchmark --full   (full benchmark)"
	@echo ""

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ============================================================
# Convenience targets
# ============================================================

run: $(TARGET)
	@mkdir -p results
	./$(TARGET) --full

demo: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	@echo "Running valgrind memory check..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		./$(TARGET) --full

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "  ✓ Cleaned!"

help:
	@echo "Available targets:"
	@echo "  make          - Build the benchmark program"
	@echo "  make run      - Build and run full benchmark"
	@echo "  make demo     - Build and run interactive mode"
	@echo "  make valgrind - Check for memory leaks"
	@echo "  make clean    - Remove build artifacts"
