CC = gcc -std=gnu99

server:server.o util.o epoll.o http_connection.o http_request.o http_parse.o http_response.o threadpool.o rio.o time_wheel.o
	$(CC) -w -g -o server *.o -lpthread

server.o:server.c
	$(CC) -w -g -c server.c

epoll.o:epoll.c
	$(CC) -w -g -c epoll.c

util.o:util.c
	$(CC) -w -g -c util.c

http_connection.o:http_connection.c
	$(CC) -w -g -c http_connection.c

http_parse.o:http_parse.c
	$(CC) -w -g -c http_parse.c

http_request.o:http_request.c
	$(CC) -w -g -c http_request.c

http_response.o:http_response.c
	$(CC) -w -g -c http_response.c

rio.o:rio.c
	$(CC) -w -g -c rio.c

time_wheel.o:time_wheel.c
	$(CC) -w -g -c time_wheel.c

threadpool.o:threadpool.c
	$(CC) -w -g -c threadpool.c

clean:
	rm *.o