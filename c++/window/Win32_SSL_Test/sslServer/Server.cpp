

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>

#include <winsock2.h>
#pragma comment(lib,"ws2_32")
#include "openssl/rsa.h"
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

/*所有需要的参数信息都在此处以#define的形式提供*/
#define CERTF "server.crt" /*服务端的证书(需经CA签名)*/
#define KEYF "server.key" /*服务端的私钥(建议加密存储)*/
#define CACERT "root.crt" /*CA 的证书*/
#define PORT 6000 /*准备绑定的端口*/

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }
/*
static int
pass_cb(char *buf, int len, int verify, void *password)
{
	snprintf(buf, len, "123456");
	return strlen(buf);
}
SSL_CTX_set_default_passwd_cb(ctx, pass_cb);
等价于：
SSL_CTX_set_default_passwd_cb_userdata(ctx, "123456");
*/

int main()
{
	int err;
	SOCKET listen_sd;
	SOCKET sd;
	struct sockaddr_in sa_serv;
	struct sockaddr_in sa_cli;
	int client_len;
	SSL_CTX* ctx;
	SSL* ssl;
	X509* client_cert;
	char* str = (char*)malloc(sizeof(char) * 1024);
	char buf[4096];
	SSL_METHOD *meth;
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		printf("WSAStartup()fail:%d\n", GetLastError());
		return -1;
	}

	SSL_load_error_strings(); /*为打印调试信息作准备*/
	OpenSSL_add_ssl_algorithms(); /*初始化*/
	meth = (struct ssl_method_st *)TLSv1_server_method(); /*采用什么协议(SSLv2/SSLv3/TLSv1)在此指定*/

	ctx = SSL_CTX_new(meth);
	CHK_NULL(ctx);
	
	//SSL_VERIFY_FAIL_IF_NO_PEER_CERT    即使客户端没有发送证书 服务端和客户端也可以完成握手
	//SSL_VERIFY_PEERFAIL_IF_NO_PEER_CERT  客户端设置了就可以验证服务端的证书
	//SSL_VERIFY_NONE表示不验证
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL); /*验证与否*/   //制定证书验证方式的函数
	SSL_CTX_load_verify_locations(ctx, CACERT, NULL); /*若验证,则放置CA证书*/

	//SSL_CTX_set_default_passwd_cb_userdata(ctx, "123456");  //直接将口令设置好

	if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(3);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(4);
	}

	if (!SSL_CTX_check_private_key(ctx)) {
		printf("Private key does not match the certificate public key\n");
		exit(5);
	}

	SSL_CTX_set_cipher_list(ctx, "RC4-MD5");

	/*开始正常的TCP socket过程.................................*/
	printf("Server:Begin TCP socket...\n");

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	CHK_ERR(listen_sd, "socket");

	memset(&sa_serv, '\0', sizeof(sa_serv));
	sa_serv.sin_family = AF_INET;
	sa_serv.sin_addr.s_addr = INADDR_ANY;
	sa_serv.sin_port = htons(PORT);

	err = bind(listen_sd, (struct sockaddr*) &sa_serv, sizeof(sa_serv));
	CHK_ERR(err, "bind");

	/*接受TCP链接*/
	err = listen(listen_sd, 5);
	CHK_ERR(err, "listen");

	client_len = sizeof(sa_cli);
	sd = accept(listen_sd, (struct sockaddr*) &sa_cli, &client_len);
	CHK_ERR(sd, "accept");
	closesocket(listen_sd);

	printf("Connection from %s, port %d\n", inet_ntoa(sa_cli.sin_addr), sa_cli.sin_port);

	/*TCP连接已建立,进行服务端的SSL过程. */

	ssl = SSL_new(ctx);
	CHK_NULL(ssl);
	SSL_set_fd(ssl, sd);
	err = SSL_accept(ssl);
	printf("SSL_accept finished\n");
	CHK_SSL(err);

	/*打印所有加密算法的信息(可选)*/
	printf("SSL connection using %s\n", SSL_get_cipher(ssl));

	/*得到服务端的证书并打印些信息(可选) */
	client_cert = SSL_get_peer_certificate(ssl);
	if (client_cert != NULL) {
		printf("Client certificate:\n");

		str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
		CHK_NULL(str);
		printf("\tsubject: %s\n", str);
		/*free (str);*/
		memset(&str, '\0', sizeof(str));

		str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
		CHK_NULL(str);
		printf("\t issuer: %s\n", str);
		/*free (str);*/
		memset(&str, '\0', sizeof(str));

		X509_free(client_cert);/*如不再需要,需将证书释放 */
	}
	else
		printf("Client does not have certificate.\n");

	/* 数据交换开始,用SSL_write,SSL_read代替write,read */
	err = SSL_read(ssl, buf, sizeof(buf) - 1);
	CHK_SSL(err);
	buf[err] = '\0';
	printf("Got %d chars:'%s'\n", err, buf);

	err = SSL_write(ssl, "I hear you.", strlen("I hear you."));
	CHK_SSL(err);

	/* 收尾工作*/
	free(str);
	shutdown(sd, 2);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	return 0;
}