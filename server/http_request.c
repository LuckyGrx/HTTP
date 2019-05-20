#include "http_request.h"
#include "time_wheel.h"

void init_http_request_t(http_request_t* request, int fd, int epollfd) {
	request->epollfd = epollfd;
	request->fd = fd;

	request->state;
	request->begin = 0;
	request->end = 0;

	request->timer = NULL;
}

void request_controller(void* ptr) {
	http_request_t* request = (http_request_t*)ptr;

	// 删除定时器
	//time_wheel_del_timer(request);
	int rc;

	while (1) {
		int reco = recv(request->fd, request->buffer + request->end, 1000, 0);

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

		// 解析请求报文的请求头
		//rc = http_parse_request_header(request);
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