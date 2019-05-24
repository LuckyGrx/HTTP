#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include "head.h"
#include "util.h"
#include "rio.h"
#include "http_parse.h"

#define CONNECTION_KEEP_ALIVE 1
#define CONNECTION_CLOSE      2

typedef struct http_response {
	int fd;
	int connection;
	int status_code;
}http_response_t;

// 用key-value 表示mime_type_t
typedef struct mime_type {
	const char *type;
	const char *value;
}mime_type_t;

enum response_status {
	response_bad_request           = 400,
	response_not_found             = 404,

	response_internal_server_error = 500,
	response_not_implemented       = 501
};

void init_response_t(http_response_t* response, int fd);

void response_controller(http_response_t* response);

const char* get_reason_phrase(int status_code);

void send_response_message(http_response_t* response);

void static_file_process(http_response_t* response, const char* filename, size_t filesize);

#endif