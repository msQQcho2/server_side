#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define MAXLINE 1024
#define MAXFD 512
#define LISTENQ 1024
#define SERV_PORT 22608

int main(int argc, char **argv) {
	int listen_sock, accept_sock, client[FD_SETSIZE], yes, i, x = 0, n;
	bool flg[FD_SETSIZE];
	socklen_t sin_siz;
	fd_set fds, readfds;
	struct sockaddr_in serv, clt;
	unsigned short port;
	struct timeval waitval;
	waitval.tv_sec = 2;
	waitval.tv_usec = 500;
	if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	printf("mycall: socket established\n");
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(SERV_PORT);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes)) < 0) {
		perror("setsockopt");
		exit(1);
	}
	if(bind(listen_sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
		perror("bind");
		exit(1);
	}
	printf("mycall: binded\n");
	if(listen(listen_sock, LISTENQ) < 0) {
		perror("listen");
		exit(0);
	}
	printf("mycall: listening...\n");
	for(i = 0; i < FD_SETSIZE; ++i) client[i] = -1, flg[i] = false;
	FD_ZERO(&readfds);
	FD_SET(listen_sock, &readfds);
	while(1) {
		sin_siz = sizeof(clt);
		/*char tmp[MAXLINE];
		scanf("%s", tmp);
		for(i = 0; i < 10; ++i) printf("%d ", FD_ISSET(i, &readfds) ? 1 : 0);
		printf("\n");*/
		memcpy(&fds, &readfds, sizeof(readfds));
		n = select(MAXFD + 1, &fds, NULL, NULL, &waitval);
		if(n < 0) {
			perror("select");
			continue;
		}
		if(FD_ISSET(listen_sock, &fds)) {
			accept_sock = accept(listen_sock, (struct sockaddr *)&clt, &sin_siz);
			if(accept_sock < 0) {
				perror("accept");
				continue;
			}
			flg[x] = true;
			client[x++] = accept_sock;
			FD_SET(accept_sock, &readfds);
			printf("mycall: connected from %s. FD = %d\n", inet_ntoa(clt.sin_addr), accept_sock);
		}
		for(i = 0; i < x; ++i) {
			if(FD_ISSET(client[i], &fds)) {
				char inbuf[MAXLINE + 1], obuf[MAXLINE + 1];
				int nr;
				memset(&inbuf, 0, sizeof(inbuf));
				nr = read(client[i], inbuf, sizeof(inbuf));
				if(nr < 0) {
					perror("read");
					close(client[i]);
					continue;
				} else if(!nr) continue;
				printf("get from FD=%d:\n***\n%s\n***\n", client[i], inbuf);
                                memset(&obuf, 0, sizeof(obuf));
                                snprintf(obuf, sizeof(obuf),
                                        "HTTP/1.0 200 OK\r\n"
                                        "Content-Type: text/html\r\n"
                                        "\r\n"
                                        "<font color=red><h1>HELLO</h1></font>\r\n");
                                send(client[i], obuf, (int)strlen(obuf), 0);
                                printf("send for FD=%d:\n***\n%s\n***\n", client[i], obuf);
                                close(client[i]);
				FD_CLR(client[i], &readfds);
                                flg[i] = false;
				printf("mycall: closed socket FD=%d\n", client[i]);
			}
		}
	}
	close(listen_sock);
	exit(0);
}

