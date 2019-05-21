#include "http_request.h"
#include "time_wheel.h"
#include "http_parse.h"

void init_http_request_t(http_request_t* request, int fd, int epollfd) {
	request->epollfd = epollfd;
	request->fd = fd;

	request->state = request_line_start;
	request->begin = 0;
	request->end = 0;

	request->timer = NULL;
}

void test(http_request_t* request) {
	// 响应报文的响应首部(响应行 + 响应头)
	char response_header[BUFFER_SIZE];
	// 响应报文的响应体
	char response_body[BUFFER_SIZE];


	sprintf(header, "HTTP/1.1 %d %s"CRLF, request->status, "hhh");
	sprintf(header, "%sContent-type: text/html"CRLF, header);
	sprintf(header, "%sContent-length: %d"CRLF, header, strlen(response_body));

	rio_writen(request->fd, response_header, strlen(response_heaser));
	rio_writen(request->fd, response_body, strlen(response_body));
}

void response_controller(http_request_t* request) {
	switch (request->status) {
		case response_bad_request:
		case response_not_implemented:
			test(request);
			break;
	}
}

void request_controller(void* ptr) {
	http_request_t* request = (http_request_t*)ptr;

	char* plast = NULL;
	size_t remain_size;
	// 删除定时器
	//time_wheel_del_timer(request);

	while (1) {
		// plast指向缓冲区buffer当前可写入的第一个字节位置
		plast = &(request->buffer[request->begin % BUFFER_SIZE]);

		// remain_size 表示缓冲区当前剩余可写入的字节数
		remain_size = BUFFER_SIZE - (request->end - request->begin);

		// 从已连接描述符fd读取数据
		int reco = recv(request->fd, plast, remain_size, 0);

		// 缓冲区无空间,或者对端断开连接,则服务器断开连接
		if (reco == 0)
			goto close;
		else if (reco == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		else if (reco < 0)
			goto close;

		// 更新读到的总字节数
		request->end += reco;

		// 解析请求报文的请求行
		http_parse_request_line(request);

		// 请求行解析有问题,则发送响应报文
		response_controller(request);

		// 解析请求报文的请求头
		http_parse_request_header(request);
	}
	// 添加定时器
	//time_wheel_add_timer(reuqest, ftp_connection_shutdown, tw.slot_interval * 10);

	ftp_epoll_mod(request->epollfd, request->fd, request, EPOLLIN | EPOLLET | EPOLLONESHOT); 

	return ;

close:
	http_request_close(request);
} 

void http_request_close(http_request_t* request) {
	if (NULL == request)
		return ;
	ftp_epoll_del(request->epollfd, request->fd, request, EPOLLIN | EPOLLET | EPOLLONESHOT);// 待处理
	close(request->fd);
	free(request);
	request = NULL;
}