# Compiler
CC = gcc

# Source files
SRC = main.c dragAndDrop.c

# Output executable
TARGET = proyecto_1

# GTK 4 flags
GTK_FLAGS = $(shell pkg-config --cflags gtk4)
GTK_LIBS = $(shell pkg-config --libs gtk4)

# Compiler flags
CFLAGS = $(GTK_FLAGS) -Wall -Wextra -pedantic -z noexecstack
LDFLAGS = -Wl,-rpath=/usr/lib/x86_64-linux-gnu -Wl,-rpath=/lib/x86_64-linux-gnu

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS) $(GTK_LIBS) -Wl,--enable-new-dtags

# Clean up build files
clean:
	rm -f $(TARGET) *.o

# Phony targets
.PHONY: all clean
