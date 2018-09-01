#include<stdio.h>
#include<string.h>
#include <time.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>
struct timeval lasttime = { 0 };

int event_is_persistent = 0;

static void timeout_cb(evutil_socket_t, short event, void* arg)
{
	struct timeval NewTime = { 0 };
	struct timeval difference = { 0 };
	struct event *timeout = (struct event*)arg;
	double elapsed;

	evutil_gettimeofday((struct timeval*)&NewTime, NULL);
	evutil_timersub(&NewTime, &lasttime, &difference);
	elapsed = difference.tv_sec + (difference.tv_usec / 1.0e6);

	printf("timeout_cb called at %d: %.3f seconds elapsed.\n",
		(int)NewTime.tv_sec, elapsed);
	lasttime = NewTime;

	if (!event_is_persistent) {
		struct timeval tv;
		evutil_timerclear(&tv);
		tv.tv_sec = 3;
		event_add(timeout, &tv);
	}
}
int main(int argc, char **argv)
{
	int nFlags = 0;
	if (argc == 2 && 0 ==strcmp(argv[1], "-p"))
	{
		event_is_persistent = 1;
		nFlags = EV_PERSIST;
	}
	
	event_base* pBase = event_base_new();

	struct event TimeOut = { 0 };
	struct timeval tv = { 0 };
	event_assign(&TimeOut, pBase, -1, nFlags, timeout_cb, (void*)&TimeOut);

	evutil_timerclear(&tv);
	tv.tv_sec = 2;
	event_add(&TimeOut, &tv);

	evutil_gettimeofday(&lasttime, NULL);

	event_base_dispatch(pBase);
	event_free(&TimeOut);
	event_base_free(pBase);
	return 0;
}