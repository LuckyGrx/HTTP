#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include "head.h"
#include "util.h"
#include "rio.h"
#include "http_request.h"

enum response_status {
	response_bad_request = 400,

	response_not_implemented = 501
};

void response_controller(http_request_t* request);

const char* get_reason_phrase(int status_code);

void send_response_message(http_request_t* request);

#endif