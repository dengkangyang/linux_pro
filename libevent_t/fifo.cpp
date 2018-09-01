#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <event2/event_struct.h>
#include <event2/event.h>
#include <signal.h>



static void
signal_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event_base *base = (event_base*)arg;
	event_base_loopbreak(base);
}


static void
fifo_read(evutil_socket_t fd, short event, void *arg)
{
	char buf[255];
	int len;
	struct event *ev = (struct event*)arg;


	fprintf(stderr, "fifo_read called with fd: %d, event: %d, arg: %p\n",
		(int)fd, event, arg);

	len = read(fd, buf, sizeof(buf)-1);

	if (len <= 0) {
		if (len == -1)
			perror("read");
		else if (len == 0)
			fprintf(stderr, "Connection closed\n");
		event_del(ev);
		event_base_loopbreak(event_get_base(ev));
		return;
	}

	buf[len] = '\0';

	fprintf(stdout, "Read: %s\n", buf);
}


int main(int argc, char **argv)
{
	struct stat st = { 0 };
	const char *fifo = "event.fifo";
	if (lstat(fifo, &st) == 0)
	{
		if ((st.st_mode & S_IFMT) == S_IFREG)
		{
			errno = EEXIST;
			perror("lstat");
			exit(1);
		}
	}

	unlink(fifo);
	if (mkfifo(fifo, 0600) == -1) 
	{
		perror("mkfifo");
		exit(1);
	}

	int socket = open(fifo, O_RDONLY | O_NONBLOCK, 0);
	if (socket == -1) 
	{
		perror("open");
		exit(1);
	}

	fprintf(stderr, "Write data to %s\n", fifo);

	event_base* base = event_base_new();
	event* signal_int = evsignal_new(base, SIGINT, signal_cb, base);
	event_add(signal_int, NULL);
	event* evfifo = event_new(base, socket, EV_READ | EV_PERSIST, fifo_read,
		event_self_cbarg());

	event_add(evfifo, NULL);
	event_base_dispatch(base);
	event_base_free(base);
	close(socket);
	unlink(fifo);
	libevent_global_shutdown();
	return (0);
}