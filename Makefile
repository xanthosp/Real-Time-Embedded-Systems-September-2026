CC = gcc
CFLAGS = -Wall -O2 -pthread
LIBS = -lwebsockets -lcjson

TARGET = espx_monitor
SRC = src/main.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean: 
	rm -f $(TARGET)


