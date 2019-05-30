#include "http_response.h"
#include "http_parse.h"
#include "time_wheel.h"

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

void init_http_response_t(http_response_t* response) {
	// 默认值为Keep-Alive,保持连接不断开
	response->connection = CONNECTION_KEEP_ALIVE;
	// 默认为200,出错时被修改
	response->status_code = response_ok;
}

void response_controller(http_response_t* response, int connfd) {
	switch (response->status_code) {
		case response_bad_request:
		case response_not_found:
		case response_forbidden:
		case response_not_implemented:
			send_response_message(response, connfd);
			break;
	}
}
// 得到响应报文响应行的原因短语
const char* get_reason_phrase(int status_code) {
	switch (status_code) {
		// 2xx
		case response_ok:
			return "OK";
		// 4xx:
		case response_bad_request:
			return "Bad Request";
		case response_forbidden:
			return "Forbidden";
		case response_not_found:
			return "Not Found";
		// 5xx:
		case response_internal_server_error:
			return "Internal Server Error";
		case response_not_implemented:
			return "Method Not Implemented";
	}
}

void send_response_message(http_response_t* response, int connfd) {
	// 响应报文的响应首部(响应行 + 响应头)
	char header[BUFFER_SIZE];
	// 响应报文的响应体
	char body[BUFFER_SIZE];

	sprintf(body, "<html><head><title>server error</title></head>");
	sprintf(body, "%s<body>", body);
	sprintf(body, "%s<h1>%d: %s</h1></body></html>", body, response->status_code, get_reason_phrase(response->status_code));

	// 填充 响应报文的响应头
	sprintf(header, "HTTP/1.1 %d %s%c%c", response->status_code, get_reason_phrase(response->status_code), CR, LF);
	sprintf(header, "%sContent-type: text/html%c%c", header, CR, LF);
	sprintf(header, "%sContent-length: %d%c%c", header, strlen(body), CR, LF);
	// 填充 空行
	sprintf(header, "%s%c%c", header, CR, LF);

	rio_writen(connfd, header, strlen(header));
	rio_writen(connfd, body, strlen(body));
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

void static_file_process(http_response_t* response, int method, int connfd, const char* filename, size_t filesize) {
	char header[BUFFER_SIZE];
	char buff[BUFFER_SIZE];
	struct tm tm;
	// 返回响应报文的响应行
	sprintf(header, "HTTP/1.1 %d %s%c%c", response->status_code, get_reason_phrase(response->status_code), CR, LF);

	// 返回响应报文的响应头部
	if (response->connection == CONNECTION_KEEP_ALIVE) {
		// 返回默认的长连接模式以及超时时间
		sprintf(header, "%sConnection: keep-alive%c%c", header, CR, LF);
		sprintf(header, "%sKeep-Alive: timeout=%d%c%c", header, DEFAULT_TICK_TIME * 1000, CR, LF);
	} else if (response->connection == CONNECTION_CLOSE) {
		sprintf(header, "%sConnection: close%c%c", header, CR, LF);
	}

	// 得到文件类型并填充Content-type字段
	const char* filetype = get_file_type(strrchr(filename, '.'));
	sprintf(header, "%sContent-type: %s%c%c", header, filetype, CR, LF);
	// 通过Content-length返回文件大小
	sprintf(header, "%sContent-length: %u%c%c", header, filesize, CR, LF);

	localtime_r(&(response->last_modify_time), &tm);
	strftime(buff, BUFFER_SIZE, "%a, %d %b %Y %H:%M:%S GMT", &tm);
	sprintf(header, "%sLast-Modified: %s%c%c", header, buff, CR, LF);

	sprintf(header, "%sServer: server%c%c", header, CR, LF);
	// 空行
	sprintf(header, "%s%c%c", header, CR, LF);

	// 发送响应报文的响应头(响应行 + 响应头部)
	size_t send_len = (size_t)rio_writen(connfd, header, strlen(header));
	printf("send_len = %u\n", send_len);
	if (send_len != strlen(header)) {
		perror("Send header failed");
		return ;
	}

	// HEAD方法不需要返回响应体
	if (method == HTTP_GET || method == HTTP_POST) {
		// 打开并发送文件
		int src_fd = open(filename, O_RDONLY, 0);
		if (src_fd > 0) {
			char* src_addr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, src_fd, 0);
			close(src_fd);

			if (src_addr != NULL) {
				// 发送文件并校验完整性
				send_len = (size_t)rio_writen(connfd, src_addr, filesize);
				printf("send_len = %u\n", send_len);
				if (send_len != filesize)
					perror("Send file failed");
				munmap(src_addr, filesize);
			}
		}
	}
}

// struct stat {
//     mode_t st_mode;  //文件对应的模式,文件,目录等
// };
int file_process(http_response_t* response, const char* filename, struct stat* sbuf) {
	// 处理文件找不到错误
	if (stat(filename, sbuf) < 0) {
		response->status_code = response_not_found;
		return 1;
	}
	// sbuf结构里保存了文件的类型
	// 处理权限错误
	if (!(S_ISREG((*sbuf).st_mode)) || !(S_IRUSR & (*sbuf).st_mode)) {
		response->status_code = response_forbidden;
		return 1;
	}
	return 0;
}