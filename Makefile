CFLAGS = -Wall -Werror -g -fdiagnostics-color=always

all: server

server: server.o
	$(CC) $(CFLAGS) -o server server.o -lpthread

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

run-gui:
	python3 Firewall_GUI.py

clean:
	rm -f *.o server
