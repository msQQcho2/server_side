#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 1024

int main(int argc, char **argv) {
        int sockfd, n;
        char recvline[MAXLINE+1], buf[MAXLINE+1];
        struct addrinfo hints, *res, *tmp_res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        if(getaddrinfo(argv[1], argv[2], &hints, &res)) {
                perror("address failed");
                exit(0);
        }
        for(tmp_res = res; tmp_res != NULL; tmp_res = tmp_res->ai_next) {
                if (tmp_res->ai_family == AF_INET)
                        printf("ipv4 address : %s\n", inet_ntop(tmp_res->ai_family, &((struct sockaddr_in *)(tmp_res->ai_addr))->sin_addr, buf, sizeof(buf)));
                else if (tmp_res->ai_family == AF_INET6)
                        printf("ipv6 address : %s\n", inet_ntop(tmp_res->ai_family,&((struct sockaddr_in6 *)(tmp_res->ai_addr))->sin6_addr, buf, sizeof(buf)));
                if((sockfd = socket(tmp_res->ai_family, tmp_res->ai_socktype, tmp_res->ai_protocol)) < 0) {
                        perror("connect failed");
                        close(sockfd);
                        continue;
                };
                if(connect(sockfd, tmp_res->ai_addr, tmp_res->ai_addrlen) < 0) {
                        perror("socket failed");
                        close(sockfd);
                        continue;
                }
                while(1) {
                        char inbuf[MAXLINE+1];
                        fgets(inbuf, MAXLINE, stdin);
                        printf("input: %s\n", inbuf);
                        if(inbuf == NULL) {
                                printf("finish\n");
                                break;
                        }
                        write(sockfd, inbuf, sizeof(inbuf));
                        n = read(sockfd, recvline, MAXLINE);
                        recvline[n] = '\0';
                        fputs(recvline, stdout);
                }
                close(sockfd);
        }
        freeaddrinfo(res);
        exit(0);
}
