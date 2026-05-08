CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -pthread
LDFLAGS = -pthread

TARGET = p2p_transfer
SRC = p2p_transfer.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean install
