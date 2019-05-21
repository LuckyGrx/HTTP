#include "http_response.h"
#include "http_parse.h"

void response_controller(http_request_t* request) {
	switch (request->status_code) {
		case response_bad_request:
		case response_not_implemented:
			send_response_message(request);
			break;
	}
}

const char* get_reason_phrase(int status_code) {
	switch (status_code) {
		// 4xx:
		case response_bad_request:
			return "Interval Server Error";
		// 5xx:
		case response_not_implemented:
			return "Method Not Implemented";
		// 2xx:
		default:
			return "OK";
	}
}

void send_response_message(http_request_t* request) {
	// 响应报文的响应首部(响应行 + 响应头)
	char header[BUFFER_SIZE];
	// 响应报文的响应体
	char body[BUFFER_SIZE];


	// 填充 响应报文的响应头
	sprintf(header, "HTTP/1.1 %d %s%c%c", request->status_code, get_reason_phrase(request->status_code), CR, LF);
	sprintf(header, "%sContent-type: text/html%c%c", header, CR, LF);
	sprintf(header, "%sContent-length: %d%c%c", header, strlen(body), CR, LF);
	// 填充 空行
	sprintf(header, "%s%c%c", header, CR, LF);

	rio_writen(request->fd, header, strlen(header));
	rio_writen(request->fd, body, strlen(body));
}