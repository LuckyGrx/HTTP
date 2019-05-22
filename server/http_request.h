#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include "head.h"
#include "util.h"
#include "rio.h"

#define BUFFER_SIZE 4096 //读缓冲区大小

typedef struct http_request {
	int               epollfd;
	int               fd;
	char              buffer[BUFFER_SIZE];

	size_t            begin;         
	size_t            end;         

	int               state;
	int               method;


	void*             request_begin;
	void*             request_end;

	void*             method_begin;
	void*             method_end;

	void*             uri_begin;
	void*             uri_end;

	int               status_code;

	int               version;             // HTTP的版本

	void*             timer;
}http_request_t;

void init_request_t(http_request_t* connection, int fd, int epollfd);

void request_controller(void*);

#endif