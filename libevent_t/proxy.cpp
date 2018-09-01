#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <event2/util.h>
#include <netinet/in.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <assert.h>
#include <event2/buffer.h>
/*#include <event2/bufferevent_ssl.h>*/

static struct event_base *base;
static struct sockaddr_storage listen_on_addr;
static struct sockaddr_storage connect_to_addr;
static int connect_to_addrlen;
static int use_wrapper = 1;

#define MAX_OUTPUT (512*1024)
static void close_on_finished_writecb(struct bufferevent *bev, void *ctx);
static void eventcb(struct bufferevent *bev, short what, void *ctx);
static void drained_writecb(struct bufferevent *bev, void *ctx);



static void readcb(struct bufferevent *bev, void *ctx)
{
	bufferevent* partner = (bufferevent*)ctx;
	evbuffer *src = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(src);
	if (NULL == partner)
	{
		evbuffer_drain(src, len);
		return;
	}

	evbuffer* dst = bufferevent_get_output(partner);
	evbuffer_add_buffer(dst, src);

	if (evbuffer_get_length(dst) >= MAX_OUTPUT)
	{
		/*We're giving the other side data faster than it can
		* pass it on.Stop reading here until we have drained the
		* other side to MAX_OUTPUT / 2 bytes*/
		bufferevent_setcb(partner, readcb, drained_writecb, eventcb, bev);
		bufferevent_setwatermark(partner, EV_WRITE, MAX_OUTPUT / 2, MAX_OUTPUT);
		bufferevent_disable(bev, EV_READ);
	}
}

static void drained_writecb(struct bufferevent *bev, void *ctx)
{
	struct bufferevent *partner = (bufferevent*)ctx;

	/* We were choking the other side until we drained our outbuf a bit.
	* Now it seems drained. */

	bufferevent_setcb(bev, readcb, NULL, eventcb, partner);
	bufferevent_setwatermark(bev, EV_WRITE, 0, 0);
	if (NULL != partner)
	{
		bufferevent_enable(partner, EV_READ);
	}
}

static void	accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
struct sockaddr *a, int slen, void *p)
{
	bufferevent* b_in = bufferevent_socket_new(base, fd, \
		BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

	bufferevent* b_out = bufferevent_socket_new(base, -1, \
		BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

	assert(b_in && b_out);

	if (bufferevent_socket_connect(b_out, (struct sockaddr*)&connect_to_addr,
		connect_to_addrlen)<0)
	{
		bufferevent_free(b_out);
		bufferevent_free(b_in);
		return;
	}

	bufferevent_setcb(b_in, readcb, NULL, eventcb, b_out);
	bufferevent_setcb(b_out, readcb, NULL, eventcb, b_in);

	bufferevent_enable(b_in, EV_READ | EV_WRITE);
	bufferevent_enable(b_out, EV_READ | EV_WRITE);
}

static void close_on_finished_writecb(struct bufferevent *bev, void *ctx)
{
	struct evbuffer *b = bufferevent_get_output(bev);

	if (evbuffer_get_length(b) == 0) {
		bufferevent_free(bev);
	}
}

static void eventcb(struct bufferevent *bev, short what, void *ctx)
{
	struct bufferevent *partner = (bufferevent*)ctx;

	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
	{
		// 		if (what & BEV_EVENT_ERROR) 
		// 		{
		// 			unsigned long err;
		// 			while ((err = (bufferevent_get_openssl_error(bev)))) 
		// 			{
		// 				const char *msg = (const char*)
		// 					ERR_reason_error_string(err);
		// 				const char *lib = (const char*)
		// 					ERR_lib_error_string(err);
		// 				const char *func = (const char*)
		// 					ERR_func_error_string(err);
		// 				fprintf(stderr,
		// 					"%s in %s %s\n", msg, lib, func);
		// 			}
		// 			if (errno)
		// 				perror("connection error");
		// 		}
		if (partner)
		{
			/* Flush all pending data */
			readcb(bev, ctx);

			if (evbuffer_get_length(bufferevent_get_output(partner))) {
				/* We still have to flush data from the other
				* side, but when that's done, close the other
				* side. */
				bufferevent_setcb(partner, NULL, close_on_finished_writecb, eventcb, NULL);
				bufferevent_disable(partner, EV_READ);
			}
			else
			{
				/* We have nothing left to say to the other
				* side; close it. */
				bufferevent_free(partner);
			}
		}
		bufferevent_free(bev);
	}
}



static void syntax(void)
{
	fputs("   le-proxy 127.0.0.1:8888 1.2.3.4:80\n", stderr);
	exit(1);
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		syntax();
	}

	memset(&listen_on_addr, 0, sizeof(struct sockaddr_storage));
	int socklen = sizeof(listen_on_addr);
	if (evutil_parse_sockaddr_port(argv[1], (struct sockaddr*)&listen_on_addr,
		&socklen)<0)
	{
		int p = atoi(argv[1]);
		struct sockaddr_in *sin = (struct sockaddr_in*)&listen_on_addr;
		if (p < 1 || p > 65535)
			syntax();
		sin->sin_port = htons(p);
		sin->sin_addr.s_addr = htonl((u_long)0x7f000001);
		sin->sin_family = AF_INET;
		socklen = sizeof(struct sockaddr_in);
	}

	memset(&connect_to_addr, 0, sizeof(connect_to_addr));
	connect_to_addrlen = sizeof(connect_to_addr);
	if (evutil_parse_sockaddr_port(argv[2], (struct sockaddr*)&connect_to_addr, &connect_to_addrlen) < 0)
	{
		syntax();
	}

	base = event_base_new();
	if (NULL == base)
	{
		perror("event_base_new()");
		return 1;
	}

	evconnlistener* listener = evconnlistener_new_bind(base, accept_cb, NULL, \
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_REUSEABLE,
		-1, (struct sockaddr*)&listen_on_addr, socklen);

	if (NULL == listener)
	{
		fprintf(stderr, "Couldn't open listener.\n");
		event_base_free(base);
		return 1;
	}

	event_base_dispatch(base);

	evconnlistener_free(listener);
	event_base_free(base);
		
	return 0;
}