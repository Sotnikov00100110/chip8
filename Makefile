# Назва виконуваного файлу
TARGET = chip8

# Компілятор
CC = clang
CFLAGS = -Wall -Wextra -std=c11 -g -fsanitize=address

SRC_DIR = src
OBJ_DIR = obj

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $@ -fsanitize=address

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
