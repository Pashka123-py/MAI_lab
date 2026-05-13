CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGET = app

OBJ = main.o udt.o sort.o

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

main.o: main.c udt.h sort.h
	$(CC) $(CFLAGS) -c main.c

udt.o: udt.c udt.h
	$(CC) $(CFLAGS) -c udt.c

sort.o: sort.c sort.h udt.h
	$(CC) $(CFLAGS) -c sort.c

clean:
	del /Q *.o *.exe