all:
	gcc -Wall -c commom.c
	gcc -Wall client_italo.c commom.o -o client
	gcc -Wall server_italo.c commom.o -o server

clean:
	@rm -rf *.o