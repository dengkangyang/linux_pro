#include <string.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/event.h>

static const std::string strMsg = "Hello World\n";
static const int PORT = 9995;

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, 
	struct sockaddr *sa, int socklen, void *user_data);

static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);

int main()
{
	event_base* pBase = NULL;
	evconnlistener* pListener = NULL;
	event* pSignal_event = NULL;

	pBase = event_base_new();  //创建事件集
	if (NULL == pBase)
	{
		std::cerr << "Could not initialize libevent!\n";
		return -1;
	}

	sockaddr_in sin;
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	pListener = evconnlistener_new_bind(pBase, listener_cb, pBase, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
		-1, (sockaddr*)&sin, sizeof(sin));
	if (NULL == pListener)
	{
		std::cerr << "Could not create listener!\n";
		return -1;
	}

	pSignal_event = evsignal_new(pBase, SIGINT, signal_cb, (void*)pBase);
	if (NULL == pSignal_event)
	{
		std::cerr << "Could not create signal!\n";
		return -1;
	}

	event_base_dispatch(pBase);
	evconnlistener_free(pListener);
	event_free(pSignal_event);
	event_base_free(pBase);
	std::cout << "done!" << std::endl;

	return 0;
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
struct sockaddr *sa, int socklen, void *user_data)
{
	event_base *pBase = reinterpret_cast<event_base*>(user_data);
	bufferevent *pDev = bufferevent_socket_new(pBase, fd, BEV_OPT_CLOSE_ON_FREE);
	if (NULL == pDev)
	{
		std::cerr << "Error constructing bufferevent!\n";
		event_base_loopbreak(pBase);
		return;
	}

	bufferevent_setcb(pDev, NULL, conn_writecb, conn_eventcb, NULL);
	bufferevent_enable(pDev, EV_WRITE);
	bufferevent_disable(pDev, EV_READ);

	bufferevent_write(pDev, strMsg.c_str(), strMsg.length());
}

static void conn_writecb(struct bufferevent *pDev, void *user_data)
{
	evbuffer *output = bufferevent_get_output(pDev);
	if (evbuffer_get_length(output) == 0)
	{
		std::cout << "flush answer!";
		bufferevent_free(pDev);
	}
}

static void conn_eventcb(struct bufferevent *pDev, short events, void *user_data)
{
	if (events & BEV_EVENT_EOF)
	{
		std::cout << "Connect closed\n";
	}
	else if (events & BEV_EVENT_ERROR)
	{
		std::cerr << "connect error: " << strerror(errno) << std::endl;
	}

	bufferevent_free(pDev);
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	event_base *pBase = reinterpret_cast<event_base*>(user_data);
	timeval delay = { 2, 0 };
	std::cout << "Caught an interrupt signal; exiting cleanly in two seconds.\n";
	event_base_loopexit(pBase, &delay);
}
