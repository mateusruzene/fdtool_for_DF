# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 \
         -Isrc/parser -Isrc/closure -Isrc/mincover -Isrc/keys -Isrc/normalform

# Root src directory
SRC_DIR = src

# Subdirectories
PARSER_DIR = $(SRC_DIR)/parser
CLOSURE_DIR = $(SRC_DIR)/closure
MINCOVER_DIR = $(SRC_DIR)/mincover
KEYS_DIR = $(SRC_DIR)/keys
NORMALFORM_DIR = $(SRC_DIR)/normalform

# main.c está na raiz
MAIN = main.c

# Object output directory
OBJ_DIR = objs

# All source files
SRCS = \
    $(PARSER_DIR)/parser.c \
    $(CLOSURE_DIR)/closure.c \
    $(MINCOVER_DIR)/mincover.c \
    $(KEYS_DIR)/keys.c \
    $(NORMALFORM_DIR)/normalform.c \
    $(MAIN)

# Convert SRCS to /objs/*.o while keeping folder structure
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

# Binary output
TARGET = fdtool

all: $(TARGET)

# Link final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build .o in /objs folder, creating subfolders automatically
$(OBJ_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
