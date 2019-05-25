#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include "head.h"
#include "http_response.h"
#include "http_request.h"

typedef struct http_connection {
    int epollfd;
    int fd;
    http_request_t request;
    http_response_t response;

	void*             timer;
}http_connection_t;

void init_http_connection_t(http_connection_t* connection, int fd, int epollfd);

void connection_controller(void*);

void http_connection_close(http_connection_t* connection);

#endif