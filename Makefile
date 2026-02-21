TARGET := pgBrowsr

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

CC := gcc

CFLAGS := -Wall -Wextra -std=c11 -I$(SRC_DIR) -MMD -MP
LDFLAGS := 

# GTK4
GTK_CFLAGS := $(shell pkg-config --cflags gtk4)
GTK_LDFLAGS := $(shell pkg-config --libs gtk4)

# libpq
LIBPQ_CFLAGS := -I$(shell pg_config --includedir)
LIBPQ_LDFLAGS := -lpq

CFLAGS += $(GTK_CFLAGS) $(LIBPQ_CFLAGS)
LDFLAGS += $(GTK_LDFLAGS) $(LIBPQ_LDFLAGS)

DEBUG ?= 0

ifeq ($(DEBUG),1)
	CFLAGS += -g -O0
else
	CFLAGS += -O2
endif

.PHONY: all clean run format

all: $(BIN_DIR)/$(TARGET)

run: all
	./$(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

format:
	clang-format -i $(SRC_DIR)/*.c $(SRC_DIR)/*.h

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Include automatically generated dependency files
-include $(OBJ_FILES:.o=.d)