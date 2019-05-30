#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include "head.h"

#define BUFFER_SIZE 4096 //读缓冲区大小

typedef struct http_request {
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

	int               version;             // HTTP的版本

	void*             key_begin;
	void*             key_end;
	void*             value_begin;
	void*             value_end;


	char*             if_none_match_begin;
	char*             if_none_match_end;
	char*             if_modified_since_begin;
	char*             if_modified_since_end;

}http_request_t;

void init_http_request_t(http_request_t* request);

#endif