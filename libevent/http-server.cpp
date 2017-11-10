#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>

void generic_cb(struct evhttp_request* req, void* arg)
{
// 	struct evbuffer* inb = evhttp_request_get_input_buffer(req);
// 	char szTemp[512] = { 0 };
// 	evbuffer_remove(inb, szTemp, evbuffer_get_length(inb));
// 	printf("get data %s\n", szTemp);
	char* s = "This is the generic buf";
	evbuffer_add(req->output_buffer, s, strlen(s));
	evhttp_send_reply(req, 200, "ok", NULL);
}

void testcb(struct evhttp_request* req, void* arg)
{
	char* s = "This is the test buf";
	evbuffer_add(req->output_buffer, s, strlen(s));
	evhttp_send_reply(req, 200, "OK", NULL);
}

int main()
{
	short http_port = 8081;
	char* http_addr = "192.168.180.137";

	struct event_base* base = event_base_new();
	struct evhttp* http_server = evhttp_new(base);
	if (NULL == http_server)
	{
		return -1;
	}

	int ret = evhttp_bind_socket(http_server, http_addr, http_port);
	if (0 != ret)
	{
		return -1;
	}

	evhttp_set_cb(http_server, "/test", testcb, NULL);
	evhttp_set_gencb(http_server, generic_cb, NULL);
	printf("http server start ok!\n");
	event_base_dispatch(base);
	evhttp_free(http_server);
	event_base_free(base);

	return 0;
}