#ifndef __HTTP_PARSE_H__
#define __HTTP_PARSE_H__

#include "head.h"
#include "http_request.h"
#include "http_response.h"

#define HTTP_GET       1
#define HTTP_POST      2
#define HTTP_HEAD      3

#define CR '\r'
#define LF '\n'

enum request_message_parse_state {
	request_line_start,
	request_line_in_method,
	request_line_in_space_before_uri,
	request_line_in_uri,
	request_line_in_space_before_version,
	request_line_in_version_H,
	request_line_in_version_HT,
	request_line_in_version_HTT,
	request_line_in_version_HTTP,
	request_line_in_version_slot,
	request_line_in_version_first_digit,
	request_line_in_version_dot,
	request_line_in_version_second_digit,
	request_line_in_CR,

	request_header_start,
	request_header_in_key,
	request_header_in_colon,
	request_header_in_space_before_value,
	request_header_in_value,
	request_header_in_CR,
	request_header_in_CRLF,
	request_header_in_CRLFCR
};


//
void http_parse_request_line(http_request_t* request);

void http_parse_request_header(http_request_t* request);

#endif