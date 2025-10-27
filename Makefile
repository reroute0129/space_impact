CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -I$(HOME)/local/include -I./include
LDFLAGS = -L$(HOME)/local/lib -lGL -lGLEW -lglfw -lm

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = .

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET = $(BIN_DIR)/space_impact

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/* $(TARGET)

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	cp $(TARGET) $(PREFIX)/bin/

uninstall:
	rm -f $(PREFIX)/bin/$(notdir $(TARGET))

.PHONY: all clean run install uninstall
