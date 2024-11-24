all:
	gcc -Wall -c commom.c
	gcc -Wall client_italo.c commom.o -o client
	gcc -Wall server_italo.c commom.o -o server
	gcc -Wall -lpthread server-mt.c commom.o -o server-mt

clean:
	rm -rf *.o client server