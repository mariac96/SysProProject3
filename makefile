
CC=gcc

CFLAGS=-g

all: monitorServer\
		travelMonitorClient

monitorServer: main.o functions.o citizens.o country.o hash_funcs.o hashing.o skiplist.o virus.o sockets.o requests.o bloom.o threads.o
	$(CC) main.o functions.o citizens.o country.o hash_funcs.o hashing.o skiplist.o virus.o sockets.o requests.o bloom.o threads.o -o monitorServer -lm -lpthread

travelMonitorClient: parent.o bloom.o sockets.o hash_funcs.o requests.o
	$(CC) parent.o bloom.o sockets.o hash_funcs.o requests.o -o travelMonitorClient -lm

threads.o: threads.c threads.h skiplist.h functions.h
	$(CC) $(CFLAGS) -c threads.c

parent.o: parent.c bloom.h sockets.h hash_funcs.h requests.h
	$(CC) $(CFLAGS) -c parent.c

bloom.o: bloom.c bloom.h hash_funcs.h
	$(CC) $(CFLAGS) -c bloom.c

requests.o: requests.c requests.h bloom.h
	$(CC) $(CFLAGS) -c requests.c

main.o: main.c functions.h hashing.h sockets.h requests.h threads.h
	$(CC) $(CFLAGS) -c main.c

functions.o: functions.c citizens.h skiplist.h virus.h sockets.h requests.h
	$(CC) $(CFLAGS) -c functions.c sockets.c

citizens.o: citizens.c citizens.h hashing.h country.h virus.h
	$(CC) $(CFLAGS) -c citizens.c

country.o: country.c country.h
	$(CC) $(CFLAGS) -c country.c

hash_funcs.o: hash_funcs.c hash_funcs.h
	$(CC) $(CFLAGS) -c hash_funcs.c

hashing.o: hashing.c hashing.h citizens.h virus.h country.h
	$(CC) $(CFLAGS) -c hashing.c

skiplist.o: skiplist.c skiplist.h citizens.h
	$(CC) $(CFLAGS) -c skiplist.c

virus.o: virus.c virus.h hash_funcs.h hashing.h skiplist.h -lm
	$(CC) $(CFLAGS) -c virus.c

sockets.o: sockets.c sockets.h
		$(CC) $(CFLAGS) -c sockets.c -lm

clean:
	rm *.o travelMonitorClient
