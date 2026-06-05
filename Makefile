# Makefile for 2D Graphics Editor (Windows / MinGW)
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g
LDFLAGS = -lm
TARGET  = graphics_editor.exe
SRC     = graphics_editor.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	del /f $(TARGET) 2>NUL || rm -f $(TARGET)
