#include "http_parse.h"

// 解析请求报文的请求行
void http_parse_request_line(http_request_t* request) {
	char ch, *p, *m;
	size_t pi;
	for (pi = request->begin; pi < request->end; ++pi) {
		p = &(request->buffer[pi % BUFFER_SIZE]);
		ch = *p;

		switch (request->state) {
			case request_line_start:
				request->request_begin = p;
				switch (ch) {
					case 'G': // 暂时支持GET,POST,HEAD三种请求方法
					case 'P':
					case 'H':
						request->method_begin = p;

						request->state = request_line_in_method;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_method:
				switch (ch) {
					case ' ':
						request->method_end = p;
						m = request->method_begin;
						switch (p - m) {
							case 3:
								if (m[0] == 'G' && m[1] == 'E' && m[2] == 'T') {
									request->method = HTTP_GET;
									break;
								}
							case 4:
								if (m[0] == 'P' && m[1] == 'O' && m[2] == 'S' && m[3] == 'T') {
									request->method = HTTP_POST;
									break;
								} else if (m[0] == 'H' && m[1] == 'E' && m[2] == 'A' && m[3] == 'D') {
									request->method = HTTP_HEAD;
									break;
								}
							default:
								request->status = response_not_implemented;
								return ;
						}
						request->state = request_line_in_space_before_uri;
						break;
					default:
						break;
				}
				break;
			// uri 必须要以 / 开始
			case request_line_in_space_before_uri:
				switch (ch) {
					case '/':
						request->state = request_line_in_uri;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_uri:
				switch (ch) {
					case ' ':
						request->state = request_line_in_space_before_version;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_space_before_version:
				switch (ch) {
					case 'H':
						request->state = request_line_in_version_H;
					default:
						request->status = response_bad_request;
						return ;

				}
				break;
			case request_line_in_version_H:
				switch (ch) {
					case 'T':
						request->state = request_line_in_version_HT;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_version_HT:
				switch (ch) {
					case 'T':
						request->state = request_line_in_version_HTT;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_version_HTT:
				switch (ch) {
					case 'P':
						request->state = request_line_in_version_HTTP;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_version_HTTP:
				switch (ch) {
					case '/':
						request->state = request_line_in_version_slot;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_version_slot:
				switch (ch) {
					case '1':
						request->state = request_line_in_version_first_digit;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_version_first_digit:
				switch (ch) {
					case '.':
						request->state = request_line_in_version_dot;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			// 支持 HTTP1.0 和 HTTP1.1
			case request_line_in_version_dot:
				switch (ch) {
					case '0':
					case '1':
						request->state = request_line_in_version_second_digit;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_version_second_digit:
				switch (ch) {
					case '\r':
						request->state = request_line_in_CR;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_line_in_CR:
				switch (ch) {
					case '\n':
						request->state = request_header_start;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			default:
				request->status = response_bad_request;
				return ;
		}
	}
	request->begin = request->end;
}

// 解析请求报文的请求头
void http_parse_request_header(http_request_t* request) {
	char ch, *p, *m;
	size_t pi;
	for (pi = request->begin; pi < request->end; ++pi) {
		p = &(request->buffer[pi % BUFFER_SIZE]);
		ch = *p;

		switch(request->state) {
			case request_header_start:
				switch (ch) {
					case CR:
						request->state = request_header_in_CRLFCR;
						break;
					default:
						request->state = request_header_in_key;
						break;
				}
				break;
			case request_header_in_key:
				switch (ch) {
					case ':':
						request->state = request_header_in_colon;
						break;
				}
				break;
			case request_header_in_colon:
				switch (ch) {
					case ' ':
						request->state = request_header_in_space_before_value;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_header_in_space_before_value:
				request->state = request_header_in_value;
				break;
			case request_header_in_value:
				switch (ch) {
					case CR:
						request->state = request_header_in_CR;
						break;
				}
				break;
			case request_header_in_CR:
				switch (ch) {
					case LF:
						request->state = request_header_in_CRLF;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
			case request_header_in_CRLF:
				switch (ch) {
					case CR:
						request->state = request_header_in_CRLFCR;
						break;
					default:
						request->state = request_header_in_key;
						break;
				}
				break;
			case request_header_in_CRLFCR:
				switch (ch) {
					case LF:
						request->state = request_line_start;
						break;
					default:
						request->status = response_bad_request;
						return ;
				}
				break;
		}
	}
	request->begin = request->end;
}