CC = gcc
CFLAGS =
TARGET = ssu
OBJECTS = main.o ext2layer.o tree.o print.o help.o exit.o
HEADERS = api.h base.h memhelper.h ext2layer.h

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

main.o : main.c $(HEADERS)
	$(CC) $(CFLAGS) -c $^

ext2layer.o : ext2layer.c $(HEADERS)
	$(CC) $(CFLAGS) -c $^

tree.o : tree.c $(HEADERS)
	$(CC) $(CFLAGS) -c $^

print.o : print.c $(HEADERS)
	$(CC) $(CFLAGS) -c $^

help.o : help.c $(HEADERS)
	$(CC) $(CFLAGS) -c $^

exit.o : exit.c $(HEADERS)
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.gch  *.o $(TARGET)
