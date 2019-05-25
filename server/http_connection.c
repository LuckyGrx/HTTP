#include "http_connection.h"
#include "http_parse.h"
#include "time_wheel.h"

void init_http_connection_t(http_connection_t* connection, int fd, int epollfd) {
    connection->epollfd = epollfd;
    connection->fd = fd;
    init_http_request_t(&(connection->request));
    init_http_response_t(&(connection->response));
	connection->timer = NULL;
}

void connection_controller(void* ptr) {
	http_connection_t* connection = (http_connection_t*)ptr;
	http_request_t* request = &(connection->request);
	http_response_t* response = &(connection->response);

	char filename[BUFFER_SIZE];
	char query_string[BUFFER_SIZE];
	char* plast = NULL;
	size_t remain_size;
	struct stat sbuf;

	int rc;
	// 删除定时器
	time_wheel_del_timer(connection);

	while (1) {
		// plast指向缓冲区buffer当前可写入的第一个字节位置
		plast = &(request->buffer[request->begin % BUFFER_SIZE]);

		// remain_size 表示缓冲区当前剩余可写入的字节数
		remain_size = BUFFER_SIZE - (request->end - request->begin);

		// 从已连接描述符fd读取数据
		int reco = recv(connection->fd, plast, remain_size, 0);

		// 缓冲区无空间,或者对端断开连接,则服务器断开连接
		if (reco == 0)
			goto close;
		else if (reco == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		else if (reco < 0)
			goto err;

		// 更新读到的总字节数
		request->end += reco;

		// 解析请求报文的请求行
		rc = http_parse_request_line(connection);
		if (rc == HTTP_EAGAIN)
			continue;
		else if (rc != 0) {
			// 请求行解析有问题,则发送响应报文
			response_controller(response, connection->fd);
			goto err;
		}

		// 解析请求报文的请求头
		rc = http_parse_request_header(connection);
		if (rc == HTTP_EAGAIN)
			continue;
		else if (rc != 0) {
			// 请求头解析有问题,则发送响应报文
			response_controller(response, connection->fd);
			goto err;
		}

		// 解析URI,获取文件名
		http_parse_uri(request->uri_begin, request->uri_end, filename, query_string);

		// 处理文件
		rc = file_process(response, filename, &sbuf);
		if (rc != 0) {
			// 文件不存在或权限错误,则发送响应报文
			response_controller(response, connection->fd);
			goto close;
		}

		// 处理静态文件请求
		static_file_process(response, connection->fd, filename, sbuf.st_size);

		// 要是响应报文要求短连接,则发送完报文后,关闭连接
		if (response->connection == CONNECTION_CLOSE)
			goto close;
	}
	// 添加定时器
	time_wheel_add_timer(connection, http_connection_close, tw.slot_interval * 10);

	ftp_epoll_mod(connection, EPOLLIN | EPOLLET | EPOLLONESHOT); 

	return ;

err:
close:
	http_connection_close(connection);
} 

void http_connection_close(http_connection_t* connection) {
	if (NULL == connection)
		return ;
	ftp_epoll_del(connection, EPOLLIN | EPOLLET | EPOLLONESHOT);// 待处理
	close(connection->fd);
	free(connection);
	connection = NULL;
}