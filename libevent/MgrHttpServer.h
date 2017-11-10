#ifndef _MGR_HTTP_SERVER_H
#define _MGR_HTTP_SERVER_H

#include <string>
struct  evhttp_request;
struct evkeyvalq;
class CMgrHttpServer
{
public:
	// ����http�������߳�
	static int CreateHttpServerThread();
	//libeventѭ��
	static void* HttpServerWorkerLibevent(void* arg);
	//����http����
	static void dump_request_cb(struct evhttp_request *req, void *arg);
	//����http GET ����
	static void handle_request_get(struct  evhttp_request *req, void *arg);
	//����http POST ����
	static void handle_request_post(struct  evhttp_request *req, void *arg);
	//ҵ�����
	static int decode_get_targ_position(struct evkeyvalq *params, std::string &str);
};

#endif