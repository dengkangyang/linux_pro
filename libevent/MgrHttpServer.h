#ifndef _MGR_HTTP_SERVER_H
#define _MGR_HTTP_SERVER_H

#include <string>
struct  evhttp_request;
struct evkeyvalq;
class CMgrHttpServer
{
public:
	// 创建http服务器线程
	static int CreateHttpServerThread();
	//libevent循环
	static void* HttpServerWorkerLibevent(void* arg);
	//接收http请求
	static void dump_request_cb(struct evhttp_request *req, void *arg);
	//处理http GET 请求
	static void handle_request_get(struct  evhttp_request *req, void *arg);
	//处理http POST 请求
	static void handle_request_post(struct  evhttp_request *req, void *arg);
	//业务解码
	static int decode_get_targ_position(struct evkeyvalq *params, std::string &str);
};

#endif