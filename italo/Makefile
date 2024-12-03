all:
	gcc -std=c99 -g -c commom.c
	gcc -std=c99 -g client.c commom.o -o client
	gcc -std=c99 -g server.c commom.o -o server
clean:
	rm -rf *.o client server server-mt