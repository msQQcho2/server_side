#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define MAXLINE 1024
#define LISTENQ 1024
#define SERV_PORT 22608

void *send_recv_thread (void *arg) {
	pthread_detach (pthread_self());
	int accept_sock = (int)arg;
	printf("mycall: accept_sock:%d\n", (int)arg);
        char inbuf[MAXLINE + 1], obuf[MAXLINE + 1];
        memset(&inbuf, 0, sizeof(inbuf));
        recv(accept_sock, inbuf, sizeof(inbuf), 0);
        printf("get from sock:%d:\n***\n%s\n***\n", (int)arg, inbuf);
        memset(&obuf, 0, sizeof(obuf));
        snprintf(obuf, sizeof(obuf),
                 "HTTP/1.0 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "\r\n"
                 "<font color=red><h1>HELLO</h1></font>\r\n");
        send(accept_sock, obuf, (int)strlen(obuf), 0);
        printf("send for sock:%d:\n***\n%s\n***\n", (int)arg, obuf);
	close(accept_sock);
	pthread_exit((void *)0);
}


int main(int argc, char **argv) {
	int listen_sock, accept_sock, pid, yes;
	socklen_t sin_siz;
	struct sockaddr_in serv, clt;
	unsigned  short  port;
	pthread_t thread_id;

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
	while(1) {
		sin_siz = sizeof(clt);
		accept_sock = accept(listen_sock, (struct sockaddr *)&clt, &sin_siz);
		if(accept_sock < 0) {
			if(errno != EINTR) perror("accept");
			continue;
		}
		printf("mycall: connected from %s. sock:%d\n", inet_ntoa(clt.sin_addr), accept_sock);
		if(pthread_create(&thread_id, NULL, send_recv_thread, (void *)accept_sock) != 0) {
			perror("pthread_create");
		}
		//printf("mycall: pid: %d\n", pid);
	}
	close(listen_sock);
	exit(0);
}

