#include <stdio.h>
#include <sys/socket.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <arpa/inet.h>
#define MAX_OUTPUT 2
#define MAX_LINE	512
static void drained_writecb(struct bufferevent *bev, void *ctx);
static void readcb(struct bufferevent *bev, void *ctx)
{
 	evbuffer *src = bufferevent_get_input(bev);
 	evbuffer* dst = bufferevent_get_output(bev);
	

	



// 	char line[MAX_LINE + 1];
// 	int n;
// 	evutil_socket_t fd = bufferevent_getfd(bev);
// 
// 	n = bufferevent_read(bev, line, MAX_LINE);
// 	line[n] = '\0';
// 	printf("fd=%u, read line: %s\n", fd, line);
// 
// 
// 
// 
// 	bufferevent_write(bev, line, n);
	evbuffer_add_buffer(dst, src);
	printf("readcb -- %d\n", evbuffer_get_length(dst));
	bufferevent_setcb(bev, readcb, drained_writecb, NULL, bev);
	bufferevent_setwatermark(bev, EV_WRITE, 38, 0);
	bufferevent_disable(bev, EV_READ);

	
// 	if (evbuffer_get_length(dst) >= MAX_OUTPUT)
// 	{
// 
// 		bufferevent_setcb(bev, readcb, drained_writecb, NULL, bev);
// 		bufferevent_setwatermark(bev, EV_WRITE, MAX_OUTPUT / 2, MAX_OUTPUT);
// 		bufferevent_disable(bev, EV_READ);
// 	}
}


static void drained_writecb(struct bufferevent *bev, void *ctx)
{
	struct bufferevent *partner = (bufferevent*)ctx;

	/* We were choking the other side until we drained our outbuf a bit.
	* Now it seems drained. */
	evbuffer *src = bufferevent_get_output(bev);
	printf("drained_writecb -- %d\n", evbuffer_get_length(src));
	bufferevent_setcb(bev, readcb, drained_writecb, NULL, partner);
	bufferevent_setwatermark(bev, EV_WRITE, 0, 0);
	if (NULL != partner)
	{
		bufferevent_enable(partner, EV_READ);
	}
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, 
struct sockaddr *sa, int socklen, void *user_data)
{
	printf("linsten - %s\n", inet_ntoa(((struct sockaddr_in*)sa)->sin_addr));
	event_base* pBase = (event_base*)user_data;
	bufferevent* b_in = bufferevent_socket_new(pBase, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

	bufferevent_setcb(b_in, readcb, NULL, NULL, b_in);
	bufferevent_enable(b_in, EV_READ | EV_WRITE);
	printf("----------------------\n");
}

int main()
{
	struct event_base* base = event_base_new();

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in local_addr = { 0 };
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = inet_addr("192.168.180.137");
	local_addr.sin_port = htons(6001);
	int socklen = sizeof(sockaddr_in);

	struct evconnlistener* pListener = evconnlistener_new_bind(base, listener_cb, base,
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_REUSEABLE,
		-1, (struct sockaddr*)&local_addr, socklen);

	event_base_dispatch(base);
	evconnlistener_free(pListener);
	event_base_free(base);
	return 0;

}