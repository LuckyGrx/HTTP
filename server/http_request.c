#include "http_request.h"
#include "time_wheel.h"
#include "http_parse.h"
#include "http_response.h"

void init_http_request_t(http_request_t* request, int fd, int epollfd) {
	request->epollfd = epollfd;
	request->fd = fd;

	request->state = request_line_start;
	request->begin = 0;
	request->end = 0;

	request->timer = NULL;
}

void request_controller(void* ptr) {
	http_request_t* request = (http_request_t*)ptr;
	char filename[BUFFER_SIZE];
	char query_string[BUFFER_SIZE];
	char* plast = NULL;
	size_t remain_size;
	struct stat sbuf;

	int rc;
	// 删除定时器
	time_wheel_del_timer(request);

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
		rc = http_parse_request_line(request);
		if (rc == HTTP_EAGAIN)
			continue;
		else if (rc != 0) {
			// 请求行解析有问题,则发送响应报文
			//response_controller(request);
			goto close;
		}

		// 解析请求报文的请求头
		rc = http_parse_request_header(request);
		if (rc == HTTP_EAGAIN)
			continue;
		else if (rc != 0) {
			// 请求头解析有问题,则发送响应报文
			//response_controller(request);
			goto close;
		}

		// 初始化响应报文数据结构
		http_response_t* response = (http_response_t*)calloc(1, sizeof(http_response_t));
		init_http_response_t(response, request->fd);

		// 解析URI,获取文件名
		http_parse_uri(request->uri_begin, request->uri_end, filename, query_string);

		// 处理文件
		//rc = file_process(request, filename, &sbuf);
		//if (rc != 0) {
		//	response_controller(request);
		//	goto close;
		//}

		// 处理静态文件请求
		static_file_process(response, filename, sbuf.st_size);

		// 要是响应报文要求短连接,则发送完报文后,关闭连接
		if (response->connection == CONNECTION_CLOSE)
			goto close;
	}
	// 添加定时器
	time_wheel_add_timer(request, http_request_close, tw.slot_interval * 10);

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