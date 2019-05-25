#include "http_request.h"
#include "http_parse.h"

void init_http_request_t(http_request_t* request) {
	request->state = request_line_start;
	request->begin = 0;
	request->end = 0;
}
