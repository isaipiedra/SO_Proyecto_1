# Compiler
CC = gcc

# Source files
SRC = main.c dragAndDrop.c fileExplorer.c utils.c outputNameEntry.c huffmanSerial.c huffmanParallel.c huffmanConcurrent.c huffmanCore.c huffmanIO.c navigate.c

# Output executable
TARGET = proyecto_1

# GTK 4 flags
GTK_FLAGS = $(shell pkg-config --cflags gtk4)
GTK_LIBS = $(shell pkg-config --libs gtk4)

# OPEN SSL flags
OPENSSL_FLAGS = $(shell pkg-config --cflags libcrypto)
OPENSSL_LIBS = $(shell pkg-config --libs libcrypto)

# Compiler flags
CFLAGS = $(GTK_FLAGS) -Wall -Wextra -pedantic -z noexecstack -pthread
LDFLAGS = -Wl,-rpath=/usr/lib/x86_64-linux-gnu -Wl,-rpath=/lib/x86_64-linux-gnu

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(OPENSSL_FLAGS) -o $(TARGET) $(SRC) $(LDFLAGS) $(GTK_LIBS) $(OPENSSL_LIBS) -Wl,--enable-new-dtags

# Clean up build files
clean:
	rm -f $(TARGET) *.o

# Phony targets
.PHONY: all clean