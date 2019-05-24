#include "http_response.h"

mime_type_t mime[] = {
	{".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/msword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {NULL ,"text/plain"}
};

void init_http_response_t(http_response_t* response, int fd) {
	response->fd = fd;
	// 默认值为Keep-Alive,保持连接不断开
	response->connection = CONNECTION_KEEP_ALIVE;
	// 默认为200,出错时被修改
	response->status_code = 200;
}

void response_controller(http_response_t* response) {
	switch (response->status_code) {
		case response_bad_request:
		case response_not_found:
		case response_not_implemented:
			send_response_message(response);
			break;
	}
}
// 得到响应报文响应行的原因短语
const char* get_reason_phrase(int status_code) {
	switch (status_code) {
		// 4xx:
		case response_bad_request:
			return "Bad Request";
		case response_not_found:
			return "Not Found";
		// 5xx:
		case response_internal_server_error:
			return "Internal Server Error";
		case response_not_implemented:
			return "Method Not Implemented";
		// 2xx:
		default:
			return "OK";
	}
}

void send_response_message(http_response_t* response) {
	// 响应报文的响应首部(响应行 + 响应头)
	char header[BUFFER_SIZE];
	// 响应报文的响应体
	char body[BUFFER_SIZE];


	// 填充 响应报文的响应头
	sprintf(header, "HTTP/1.1 %d %s%c%c", response->status_code, get_reason_phrase(response->status_code), CR, LF);
	sprintf(header, "%sContent-type: text/html%c%c", header, CR, LF);
	sprintf(header, "%sContent-length: %d%c%c", header, strlen(body), CR, LF);
	// 填充 空行
	sprintf(header, "%s%c%c", header, CR, LF);

	rio_writen(response->fd, header, strlen(header));
	rio_writen(response->fd, body, strlen(body));
}

static const char* get_file_type(const char* type) {
	// 将type和索引表中后缀(type)进行比较,返回类型用于填充Content-Type字段
	for (int i = 0; mime[i].type != NULL; ++i) {
		if (strcmp(type, mime[i].type) == 0)
			return mime[i].value;
	}
	//未识别则返回"text/plain"
	return "text/plain";
}

void static_file_process(http_response_t* response, const char* filename, size_t filesize) {
	char header[BUFFER_SIZE];
	
	// 返回响应报文的响应行
	sprintf(header, "HTTP/1.1 %d %s%c%c", response->status_code, get_reason_phrase(response->status_code), CR, LF);

	// 返回响应报文的响应头部

	if (response->connection == CONNECTION_KEEP_ALIVE) {
		// 返回默认的长连接模式以及超时时间
		sprintf(header, "%sConnection: keep-alive%c%c", header, CR, LF);
		//sprintf(header, "%sKeep-Alive: timeout=%d%c%c", header, TIMEOUT_DEFAULT, CR, LF);
	} else if (response->connection == CONNECTION_CLOSE) {
		sprintf(header, "%sConnection: close%c%c", header, CR, LF);
	}
	// 得到文件类型并填充Content-type字段
	const char* filetype = get_file_type(strrchr(filename, '.'));
	sprintf(header, "%sContent-type: %s%c%c", header, filetype, CR, LF);
	// 通过Content-length返回文件大小
	sprintf(header, "%sContent-length: %uz%c%c", header, filesize, CR, LF);

	sprintf(header, "%sServer: HTTP%c%c", header, CR, LF);
	
	// 空行
	sprintf(header, "%s%c%c", header);

	// 发送响应报文的响应头(响应行 + 响应头部)
	size_t send_len = (size_t)rio_writen(response->fd, header, strlen(header));


	// 打开并发送文件
	int src_fd = open(filename, O_RDONLY, 0);
	char* src_addr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, src_fd, 0);
	close(src_fd);

	// 发送文件
	send_len = (size_t)rio_writen(response->fd, src_addr, filesize);
	munmap(src_addr, filesize);
}