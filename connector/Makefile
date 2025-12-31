
# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Output binary name
TARGET = poll_to_ipc

# Source files
SRC = poll_to_ipc.c

# Libraries needed (for POSIX message queues, you need to link against the real-time library)
LIBS = -lrt

# Object files
OBJ = $(SRC:.c=.o)

# Default target: build the program
all: $(TARGET)

# Link the program
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

# Compile the source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build files (object files, the binary)
clean:
	rm -f $(OBJ) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Install the program (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall the program (optional)
uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean run install uninstall
