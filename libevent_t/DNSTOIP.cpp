#include <iostream>
#include <event2/event.h>
#include <event2/dns_struct.h>
#include <event2/dns.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#ifdef EVENT__HAVE_UNISTD_H
#include <unistd.h>
#endif

static int verbose = 0;

#define u32 ev_uint32_t
#define u8 ev_uint8_t

static const char *
debug_ntoa(u32 address)
{
	static char buf[32];
	u32 a = ntohl(address);
	evutil_snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
		(int)(u8)((a >> 24) & 0xff),
		(int)(u8)((a >> 16) & 0xff),
		(int)(u8)((a >> 8) & 0xff),
		(int)(u8)((a)& 0xff));
	return buf;
}

static void main_callback(int result, char type, int count, int ttl,
void *addrs, void *orig) 
{
	char *n = (char*)orig;
	int i;
	for (i = 0; i < count; ++i) 
	{
		if (type == DNS_IPv4_A) 
		{
			printf("%s: %s\n", n, debug_ntoa(((u32*)addrs)[i]));
		}
		else if (type == DNS_PTR) 
		{
			printf("%s: %s\n", n, ((char**)addrs)[i]);
		}
	}
	if (!count) 
	{
		printf("%s: No answer (%d)\n", n, result);
	}
	fflush(stdout);
}

static void gai_callback(int err, struct evutil_addrinfo *ai, void *arg)
{
	const char *name = (char*)arg;
	int i;
	if (err) 
	{
		printf("%s: %s\n", name, evutil_gai_strerror(err));
	}
	if (ai && ai->ai_canonname)
		printf("    %s ==> %s\n", name, ai->ai_canonname);
	for (i = 0; ai; ai = ai->ai_next, ++i) 
	{
		char buf[128];
		if (ai->ai_family == PF_INET) 
		{
			struct sockaddr_in *sin = (struct sockaddr_in*)ai->ai_addr;
			evutil_inet_ntop(AF_INET, &sin->sin_addr, buf,
				sizeof(buf));
			printf("[%d] %s: %s\n", i, name, buf);
		}
		else 
		{
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)ai->ai_addr;
			evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf,
				sizeof(buf));
			printf("[%d] %s: %s\n", i, name, buf);
		}
	}
}

static void logfn(int is_warn, const char *msg) {
	if (!is_warn && !verbose)
		return;
	fprintf(stderr, "%s: %s\n", is_warn ? "WARN" : "INFO", msg);
}

static void evdns_server_callback(struct evdns_server_request *req, void *data)
{
	int i, r;
	(void)data;
	/* dummy; give 192.168.11.11 as an answer for all A questions,
	*	give foo.bar.example.com as an answer for all PTR questions. */
	for (i = 0; i < req->nquestions; ++i) {
		u32 ans = htonl(0xc0a80b0bUL);
		if (req->questions[i]->type == EVDNS_TYPE_A &&
			req->questions[i]->dns_question_class == EVDNS_CLASS_INET) {
			printf(" -- replying for %s (A)\n", req->questions[i]->name);
			r = evdns_server_request_add_a_reply(req, req->questions[i]->name,
				1, &ans, 10);
			if (r < 0)
				printf("eeep, didn't work.\n");
		}
		else if (req->questions[i]->type == EVDNS_TYPE_PTR &&
			req->questions[i]->dns_question_class == EVDNS_CLASS_INET) {
			printf(" -- replying for %s (PTR)\n", req->questions[i]->name);
			r = evdns_server_request_add_ptr_reply(req, NULL, req->questions[i]->name,
				"foo.bar.example.com", 10);
			if (r < 0)
				printf("ugh, no luck");
		}
		else
		{
			printf(" -- skipping %s [%d %d]\n", req->questions[i]->name,
				req->questions[i]->type, req->questions[i]->dns_question_class);
		}
	}

}

int main(int argc, char** argv)
{
	struct options
	{
		int reverse;
		int use_getaddrinfo;
		int servertest;
		const char* resolv_conf;
		const char* ns;
	};
	options o;
	int opt;
	event_base *pEventBase = NULL;
	evdns_base *pEvdnsBase = NULL;

	if (argc < 2) {
		fprintf(stderr, "syntax: %s [-x] [-v] [-c resolv.conf] [-s ns] hostname\n", argv[0]);
		fprintf(stderr, "syntax: %s [-T]\n", argv[0]);
		return 1;
	}

	memset(&o, 0, sizeof(o));
	while ((opt = getopt(argc, argv, "xvgc:Ts:")) != -1) {
		switch (opt) {
		case 'x': o.reverse = 1; break;
		case 'v': ++verbose; break;
		case 'g': o.use_getaddrinfo = 1; break;
		case 'T': o.servertest = 1; break;
		case 'c': o.resolv_conf = optarg; break;
		case 's': o.ns = optarg; break;
		default: fprintf(stderr, "Unknown option %c\n", opt); break;
		}
	}

	pEventBase = event_base_new();
	pEvdnsBase = evdns_base_new(pEventBase, EVDNS_BASE_INITIALIZE_NAMESERVERS | EVDNS_BASE_DISABLE_WHEN_INACTIVE);
	evdns_set_log_fn(logfn);
//	evdns_base_set_option(pEvdnsBase, "randomize-case:", "0");

	if (o.servertest == 1)
	{
		evutil_socket_t sock;
		struct sockaddr_in my_addr;
		sock = socket(PF_INET, SOCK_DGRAM, 0);
		if (sock == -1) {
			perror("socket");
			exit(1);
		}
		evutil_make_socket_nonblocking(sock);
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(10053);
		my_addr.sin_addr.s_addr = INADDR_ANY;
		if (bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
			perror("bind");
			exit(1);
		}
		evdns_add_server_port_with_base(pEventBase, sock, 0, evdns_server_callback, NULL);
	}

	if (optind < argc) 
	{
		int res;
#ifdef _WIN32
		if (o.resolv_conf == NULL && !o.ns)
			res = evdns_base_config_windows_nameservers(pEvdnsBase);
		else
#endif
		if (o.ns)
			res = evdns_base_nameserver_ip_add(pEvdnsBase, o.ns);
		else
			res = evdns_base_resolv_conf_parse(pEvdnsBase,
			DNS_OPTION_NAMESERVERS, o.resolv_conf);

		if (res < 0) {
			fprintf(stderr, "Couldn't configure nameservers");
			return 1;
		}
	}

	printf("EVUTIL_AI_CANONNAME in example = %d\n", EVUTIL_AI_CANONNAME);
	for (; optind < argc; ++optind) 
	{
		if (o.reverse) 
		{
			struct in_addr addr;
			if (evutil_inet_pton(AF_INET, argv[optind], &addr) != 1) 
			{
				fprintf(stderr, "Skipping non-IP %s\n", argv[optind]);
				continue;
			}
			fprintf(stderr, "resolving %s...\n", argv[optind]);
			evdns_base_resolve_reverse(pEvdnsBase, &addr, 0, main_callback, argv[optind]);
		}
		else if (o.use_getaddrinfo) 
		{
			printf("== -g\n");
			struct evutil_addrinfo hints;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = PF_UNSPEC;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = EVUTIL_AI_CANONNAME;
			hints.ai_socktype = SOCK_STREAM;

			fprintf(stderr, "resolving (fwd) %s...\n", argv[optind]);
			evdns_getaddrinfo(pEvdnsBase, argv[optind], NULL, &hints,
				gai_callback, argv[optind]);
		}
		else 
		{
			fprintf(stderr, "resolving (fwd) %s...\n", argv[optind]);
			evdns_base_resolve_ipv4(pEvdnsBase, argv[optind], 0, main_callback, argv[optind]);
		}
	}
	fflush(stdout);
	event_base_dispatch(pEventBase);
	return 0;
}