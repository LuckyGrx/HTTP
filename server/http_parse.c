#include "http_parse.h"

// 解析请求报文的请求行
int http_parse_request_line(http_connection_t* connection) {
	http_request_t* request = &(connection->request);    // 获得请求数据结构
	http_response_t* response = &(connection->response); // 获得响应数据结构

	char ch, *p, *m;
	size_t pi = request->begin;

	if (request->state >= request_header_start)
		return 0;

	for (; pi < request->end; ++pi) {
		p = &(request->buffer[pi % BUFFER_SIZE]);
		ch = *p;

		switch (request->state) {
			case request_line_start:
				request->request_begin = p;
				switch (ch) {
					case 'G': // 暂时支持GET,POST,HEAD三种请求方法
					case 'P':
					case 'H':
						request->state = request_line_in_method;
						request->method_begin = p;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
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
								response->status_code = response_not_implemented;
								return HTTP_PARSE_INVALID_REQUEST_LINE;
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
						request->uri_begin = p;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_uri:
				switch (ch) {
					case ' ':
						request->state = request_line_in_space_before_version;
						request->uri_end = p;
						break;
				}
				break;
			case request_line_in_space_before_version:
				switch (ch) {
					case 'H':
						request->state = request_line_in_version_H;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_version_H:
				switch (ch) {
					case 'T':
						request->state = request_line_in_version_HT;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_version_HT:
				switch (ch) {
					case 'T':
						request->state = request_line_in_version_HTT;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_version_HTT:
				switch (ch) {
					case 'P':
						request->state = request_line_in_version_HTTP;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_version_HTTP:
				switch (ch) {
					case '/':
						request->state = request_line_in_version_slot;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_version_slot:
				switch (ch) {
					case '1':
						request->state = request_line_in_version_first_digit;
						request->version = ch - '0';
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_version_first_digit:
				switch (ch) {
					case '.':
						request->state = request_line_in_version_dot;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			// 支持 HTTP1.0 和 HTTP1.1
			case request_line_in_version_dot:
				switch (ch) {
					case '0':
					case '1':
						request->state = request_line_in_version_second_digit;
						request->version = request->version * 10 + ch - '0';
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_version_second_digit:
				switch (ch) {
					case CR:
						request->state = request_line_in_CR;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
			case request_line_in_CR:
				switch (ch) {
					case LF:
						request->request_end = p;
						goto done;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_LINE;
				}
				break;
		}
	}
	request->begin = pi;
	return HTTP_EAGAIN;

done:
	request->begin = pi + 1;
	request->state = request_header_start;
	return 0;
}

// 解析请求报文的请求头
int http_parse_request_header(http_connection_t* connection) {
	http_request_t* request = &(connection->request);    // 获得请求数据结构
	http_response_t* response = &(connection->response); // 获得响应数据结构

	char ch, *p, *m;
	size_t pi = request->begin;
	for (; pi < request->end; ++pi) {
		p = &(request->buffer[pi % BUFFER_SIZE]);
		ch = *p;

		switch(request->state) {
			case request_header_start:
				switch (ch) {
					case CR:
						request->state = request_header_in_CRLFCR;
						break;
					default:
						request->key_begin = p;
						request->state = request_header_in_key;
						break;
				}
				break;
			case request_header_in_key:
				switch (ch) {
					case ':':
						request->key_end = p;
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
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_HEADER;
				}
				break;
			case request_header_in_space_before_value:
				request->value_begin = p;
				request->state = request_header_in_value;
				break;
			case request_header_in_value:
				switch (ch) {
					case CR:
						request->value_end = p;
						request->state = request_header_in_CR;
						break;
				}
				break;
			case request_header_in_CR:
				switch (ch) {
					case LF:
						get_cache_info(&request);
						request->state = request_header_in_CRLF;
						break;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_HEADER;
				}
				break;
			case request_header_in_CRLF:
				switch (ch) {
					case CR:
						request->state = request_header_in_CRLFCR;
						break;
					default:
						request->key_begin = p;
						request->state = request_header_in_key;
						break;
				}
				break;
			case request_header_in_CRLFCR:
				switch (ch) {
					case LF:
						goto done;
					default:
						response->status_code = response_bad_request;
						return HTTP_PARSE_INVALID_REQUEST_HEADER;
				}
				break;
		}
	}
	request->begin = pi;
	return HTTP_EAGAIN;

done:
	request->begin = pi + 1;
	request->state = request_line_start;
	return 0;
}

// 解析uri,获得文件名
void http_parse_uri(char *uri_begin, char *uri_end, char *filename, char *query_string) {
	for (char* p = uri_begin; p != uri_end; ++p) {
		printf("%c", *p);
	}
	printf("\n");

	*uri_end = '\0';  // 设置uri_end为空字符,否则strchr无法工作
	// 找到'?'位置,分割请求参数
	char *delim_pos = strchr(uri_begin, '?');
	int filename_length = ((delim_pos == NULL) ? (uri_end - uri_begin) : (int)(delim_pos - uri_begin));

	// 添加服务器文件根目录到filename中
	strcpy(filename, ROOTPATH);
	// 添加uri中?前面的内容到filename中
	strncat(filename, uri_begin, filename_length);

	// 添加请求参数到query_string中
	if (delim_pos != NULL)
		strcpy(query_string, delim_pos + 1);

	// 要是filename 是目录,则默认请求该目录下的index.html文件
	if (filename[strlen(filename) - 1] == '/')
		strcat(filename, "index.html");

	printf("filename = %s\n", filename);
	printf("query_string = %s\n", query_string);

	return ;
}


void get_cache_info(http_request_t** p_request) {
	http_request_t* request = *p_request;

	int key_len = request->key_end - request->key_begin;
	printf("key_len = %d\n", key_len);

	int value_len = request->value_end - request->value_begin;
	printf("value_len = %d\n", value_len);

	if (key_len == strlen("If-None-Match")) {
		if (!strncasecmp(request->key_begin, "If-None-Match", strlen("If-None-Match"))) {
			request->if_none_match_begin = (char*)(request->value_begin);
			request->if_none_match_end = (char*)(request->value_end);


			//for (char* pi = request->if_none_match_begin; pi != request->if_none_match_end; ++pi)
			//	printf("%c", *pi);
			//printf("\n");	
		}
	}

	if (key_len == strlen("If-Modified-Since")) {
		if (!strncasecmp(request->key_begin, "If-Modified-Since", strlen("If-Modified-Since"))) {
			request->if_modified_since_begin = (char*)(request->value_begin);
			request->if_modified_since_end = (char*)(request->value_end);

			//for (char* pi = request->if_modified_since_begin; pi != request->if_modified_since_end; ++pi)
			//	printf("%c", *pi);
			//printf("\n");
		}
	}
}