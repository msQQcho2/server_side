#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 1024
#define LISTENQ 1024
#define SERV_PORT 22608

void sigchld_handler(int sig) {
	int status, retval;
	do {
		retval = waitpid(-1, &status, WNOHANG);
	} while (retval > 0);
	signal(SIGCHLD, sigchld_handler);
}

int main(int argc, char **argv) {
	int listen_sock, accept_sock, pid, yes;
	socklen_t sin_siz;
	struct sockaddr_in serv, clt;
	unsigned  short  port;
	signal (SIGCHLD,  sigchld_handler);
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
			perror("accept");
			continue;
		}
		printf("mycall: connected from %s.\n", inet_ntoa(clt.sin_addr));
		pid = fork();
		if (pid < 0) {
			close(listen_sock);
			perror("fork");
			exit(1);
		} else if (pid == 0) {
			close(listen_sock);
			printf("mycall: child process:%d\n", getpid());
			char inbuf[MAXLINE + 1], obuf[MAXLINE + 1];
			memset(&inbuf, 0, sizeof(inbuf));
			recv(accept_sock, inbuf, sizeof(inbuf), 0);
			printf("get from %s:\n***\n%s\n***\n", inet_ntoa(clt.sin_addr), inbuf);
			memset(&obuf, 0, sizeof(obuf));
			snprintf(obuf, sizeof(obuf),
				"HTTP/1.0 200 OK\r\n"
				"Content-Type: text/html\r\n"
				"\r\n"
				"<font color=red><h1>HELLO</h1></font>\r\n");
			send(accept_sock, obuf, (int)strlen(obuf), 0);
			printf("send for %s:\n***\n%s\n***\n", inet_ntoa(clt.sin_addr), obuf);
			exit(0);
		}
		printf("mycall: pid: %d\n", pid);
		if(close(accept_sock) < 0) perror("close");
	}
	close(listen_sock);
	exit(0);
}

