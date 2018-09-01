#include "MgrHttpServer.h"
#include <event2/event.h>
#include <event2/event_struct.h>
/*#include <json.h>*/
#include <event2/http.h>
#include <event2/http_struct.h>
#include <pthread.h>
#include <string.h>
#include <string>
#include <event2/buffer.h>
#include <unistd.h>

#define LOG(fmt...)  do { fprintf(stderr,"%s %s ",__DATE__,__TIME__); fprintf(stderr, ##fmt); } while(0)
#define GET_TARG_POSITION "/Target/GetPosition" //
#define GET_TARG_POSITION_SIZE 19
#define GET_TARG_BASICINFO "/Target/GetInfo"    //
#define GET_TARG_BASICINFO_SIZE 15

int CMgrHttpServer::CreateHttpServerThread()
{
	struct event_base* base = event_base_new();
	if (NULL == base)
	{
		LOG("event_base_new FAILED!\n");
		return -1;
	}

	struct evhttp* http = evhttp_new(base);
	if (NULL == http)
	{
		LOG("evhttp_new FAILED!\n");
		return -1;
	}

	//接收http任意请求
	evhttp_set_gencb(http, CMgrHttpServer::dump_request_cb, NULL);
	
	struct evhttp_bound_socket* handle = evhttp_bind_socket_with_handle(http,
		"192.168.180.137", 8181);
	if (NULL == handle)
	{
		LOG("evhttp_bind_socket FAILED!\n");
		return -1;
	}

	//准备结束，启动循环线程
	int iRet = 0;
	pthread_t tidHttpServer;
	if ((iRet = pthread_create(&tidHttpServer, NULL, CMgrHttpServer::HttpServerWorkerLibevent, base)))
	{
		LOG("Create Http Server Thread Failed! -- %d", iRet);
		return -1;
	}
	
	return 0;
}


void* CMgrHttpServer::HttpServerWorkerLibevent(void* arg)
{
	struct event_base* pBase = static_cast<struct event_base*>(arg);
	if (!pBase)
	{
		LOG("Http Server thread arg FAILED!");
		return NULL;
	}
	(void)event_base_loop(pBase, 0);

	return NULL;
}

void CMgrHttpServer::dump_request_cb(struct evhttp_request *req, void *arg)
{
	switch (evhttp_request_get_command(req))
	{
	case EVHTTP_REQ_GET:
		LOG("CMD Type GET \n");
		handle_request_get(req, arg);
		break;
	case EVHTTP_REQ_POST:
		LOG("CMD Type POST \n");
		handle_request_post(req, arg);
		break;
	case EVHTTP_REQ_HEAD:
		LOG("CMD Type HEAD \n");
		break;
	case EVHTTP_REQ_PUT:
		LOG("CMD Type PUT \n");
		break;
	case EVHTTP_REQ_DELETE:
		LOG("CMD Type DELETE \n");
		break;
	case EVHTTP_REQ_OPTIONS:
		LOG("CMD Type OPTIONS \n");
		break;
	case EVHTTP_REQ_TRACE:
		LOG("CMD Type TRACE \n");
		break;
	case EVHTTP_REQ_CONNECT:
		LOG("CMD Type CONNECT \n");
		break;
	case EVHTTP_REQ_PATCH:
		LOG("CMD Type PATCH \n");
		break;
	default:
		LOG("CMD Type UnKnow \n");
		break;
	}
}

void CMgrHttpServer::handle_request_get(struct  evhttp_request *req, void *arg)
{
	//获取请求的URI
	const char* uri = evhttp_request_get_uri(req);
	LOG("Received a GET request for %s headers:", uri);

	struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
	for (struct evkeyval *header = headers->tqh_first; header; header = header->next.tqe_next)
	{
		LOG("%s :  %s", header->key, header->value);
	}

	//参数查询
	struct evkeyvalq params = { 0 };
	evhttp_parse_query(uri, &params);
	int CODE = HTTP_OK;
	std::string strOut = "";
	if (0 == strncmp(uri, GET_TARG_POSITION, GET_TARG_POSITION_SIZE))
	{
		CODE = decode_get_targ_position(&params, strOut);
	}
	else if (0 == strncmp(uri, GET_TARG_BASICINFO, GET_TARG_BASICINFO_SIZE))
	{
		LOG("Expand Get Target Info Interface, havn't implemented!");
		CODE = HTTP_NOTIMPLEMENTED;
		strOut.append("Get Target Info not implemented");
	}
	else
	{
		LOG("Reserved!");
		CODE = HTTP_NOTIMPLEMENTED;
		strOut.append("not implemented");
	}

	struct evbuffer* buf = evbuffer_new();
	evbuffer_add_printf(buf, "%s", strOut.c_str());
	//http 应答   
	evhttp_send_reply(req, CODE, "OK", buf);
	//释放资源
	evhttp_clear_headers(&params);
	evhttp_clear_headers(headers);
	evbuffer_free(buf);
}

void CMgrHttpServer::handle_request_post(struct  evhttp_request *req, void *arg)
{
	LOG("handle_request_post Reserved!");
}


int CMgrHttpServer::decode_get_targ_position(struct evkeyvalq *params, std::string &str)
{
	//JSON格式编码
// 	Json::Value root;
// 	Json::Value arrayObj;
// 	root["resultCode"] = "0";
// 	root["TargName"] = "Test";
// 	root["Latitude"] = "30.12345";
// 	root["Longitude"] = "102.1323";
// 	root["dateTime"] = 1452052317;
// 	str = root.toStyledString();
	return HTTP_OK;
}


int main()
{
	if (-1 == CMgrHttpServer::CreateHttpServerThread())
	{
		LOG("Create Http Server Thread Failed!");
		return -1;
	}
	while (1)
	{
		sleep(1000);
	}
	
	return 0;
}

