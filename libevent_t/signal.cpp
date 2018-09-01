#include <stdio.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>

int called = 0;

static void
signal_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event *signal = (struct event*)arg;

	printf("signal_cb: got signal %d\n", event_get_signal(signal));

	if (called >= 2)
		event_del(signal);

	called++;
}


int main(int argc, char **argv)
{
	event_base *pBase = event_base_new();

	//event_self_cbarg 把事件本身作为回调函数的参数
	event* pSignal = evsignal_new(pBase, SIGINT, signal_cb, event_self_cbarg());

	event_add(pSignal, NULL);
	event_base_dispatch(pBase);
	event_free(pSignal);
	event_base_free(pBase);
	return 0;
}