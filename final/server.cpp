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
#include <utility>
#include <algorithm>
#include <vector>
#include <time.h>
#include <string>

#define MAXLINE 60000
#define LISTENQ 1024
#define SERV_PORT 22608
#define MAXNOW 1000

using namespace std;

int now = 0;
char flg[MAXNOW], msg[MAXNOW][20], block[MAXNOW][22][10]; //flg -> 0: no connection, 1: preparing, 2: ready or playing, 3: finished(lose)

void *send_recv_thread (void *arg);
void *count (void *arg);
void oper (char *inbuf, char *obuf);
int getjson(char *body);
void writebody(int me, char *body);

void *send_recv_thread (void *arg) { //operation for each socket
  char chr;
  int i, pos;
  pthread_detach (pthread_self());
  int accept_sock = *((int *)arg);
  printf("mycall: accept_sock:%d\n", accept_sock);
  char *inbuf, *obuf;
  inbuf = (char*)malloc(sizeof(char) * (MAXLINE + 1));
  obuf = (char*)malloc(sizeof(char) * (MAXLINE + 1));
  memset(inbuf, 0, MAXLINE + 1);
  memset(obuf, 0, MAXLINE + 1);
  if(recv(accept_sock, inbuf, MAXLINE + 1, 0) == -1) {
    perror("recv");
    close(accept_sock);
    free(inbuf), free(obuf);
    pthread_exit((void *)0);
  }
  printf("get from sock:%d:\n***\n%s\n***\n", accept_sock, inbuf);
  oper(inbuf, obuf);
  if(send(accept_sock, obuf, (int)strlen(obuf), 0) < 1) {
    perror("send");
    close(accept_sock);
    free(inbuf), free(obuf);
    pthread_exit((void *)0);
  }
  if(strlen(obuf) > 1024) obuf[1024] = '.', obuf[1025] = '.', obuf[1026] = '.', obuf[1027] = 0;
  printf("send for sock:%d:\n***\n%s\n***\n", accept_sock, obuf);
  close(accept_sock);
  free(inbuf);
  free(obuf);
  pthread_exit((void *)0);
}

void oper(char *inbuf, char obuf[]) { //reading the request from the clients
  memset(obuf, 0, MAXLINE + 1);
  if(strlen(inbuf) < 2) return;
  char cmd[10], path[100], httpv[20], body[MAXLINE + 1], obody[MAXLINE + 1] = "";
  int i, j;
  for(i = 0; inbuf[i] != ' ' && i < strlen(inbuf); ++i) cmd[i] = inbuf[i]; //request line; method
  cmd[i] = 0;
  int pos = ++i;
  for(i = pos; inbuf[i] != ' ' && i < strlen(inbuf); ++i) path[i - pos] = inbuf[i]; //request line; URI
  path[i - pos] = 0;
  pos = ++i;
  for(i = pos; inbuf[i] != '\r' && inbuf[i] != '\n' && i < strlen(inbuf); ++i) httpv[i - pos] = inbuf[i]; //request line; version
  httpv[i - pos] = 0;
  while(inbuf[i] == '\r' || inbuf[i] == '\n') ++i;
  pos = i;
  vector<pair<string, string> > vec;
  while(pos < strlen(inbuf)) {
    char ca[100], cb[1000];
    for(i = pos; inbuf[i] != ' ' && inbuf[i] != '\r' && inbuf[i] != '\n'; ++i) {
      ca[i - pos] = inbuf[i];
    }
    if(i == pos) break; //content of line is "\r\n" then reading body
    ca[i - pos] = 0;
    pos = ++i;
    for(; inbuf[i] != '\r' && inbuf[i] != '\n'; ++i) {
      cb[i - pos] = inbuf[i];
    }
    cb[i - pos] = 0;
    string sa = ca, sb = cb;
    vec.push_back(make_pair(sa, sb)); //HTTP header, (field:, content)
    pos = i + 2; //skip for \r\n
  }
  while(inbuf[i] == '\r' || inbuf[i] == '\n') ++i;
  sort(vec.begin(), vec.end()); //optimization for using contents(not necessary)
  pos = i;
  for(i = pos; i < strlen(inbuf); ++i) body[i - pos] = inbuf[i]; //HTTP bpdy
  body[i - pos] = 0;
  char status[100] = "HTTP/1.0 200 OK", contenttype[100] = "Content-Type: text/html",
  date[100], dlen[100], ok = 1; //respons headers +alpha
  time_t t = time(NULL);
  strftime(date, sizeof(date), "Date: %a, %d %b %Y %H:%M:%S GMT", localtime(&t));
  if(!strcmp(cmd, "GET")) {
    if(!strcmp(path, "/")) {
      FILE *fp;
      fp = fopen("tetris.html", "r"); //html content
      if(fp == NULL) {
	printf("File Error\n");
	return;
      }
      char chr;
      for(i = 0; (chr = fgetc(fp)) != EOF; ++i) obody[i] = chr;
      obody[i] = 0;
      fclose(fp);
    } else if(!strcmp(path, "/me")) {
      flg[now] = 1;
      snprintf(msg[now ^ 1], sizeof(msg[now ^ 1]), "matched"); //opponent message changing
      for(i = 0; i < 22; ++i) for(j = 0; j < 10; ++j) block[now][i][j] = 0;
      snprintf(obody, sizeof(obody), "%d", now++); //return vacant number
      snprintf(contenttype, sizeof(contenttype), "Content-Type: text/plain");
    } else {
      ok = 0; //ok = 0 -> no header and no body
      snprintf(status, sizeof(status), "HTTP/1.0 404 Not Found"); //no contents
    }
  } else if(!strcmp(cmd, "POST")) {
    if(!strcmp(path, "/start")) {
      int menow = atoi(body);
      flg[menow] = 2;
      if(flg[menow ^ 1] == 2) {
	pthread_t thread_id;
	int tmpnow = menow - (menow & 1); //1st digit -> 0 (= & -2)
	if(pthread_create(&thread_id, NULL, count, (void *)&tmpnow) != 0) {
          perror("pthread_create");
        }
      }
      obody[0] = '\n', obody[1] = 0;
      snprintf(contenttype, sizeof(contenttype), "Content-Type: text/plain"); //return no content
    } else if(!strcmp(path, "/wait")) {
      int menow = atoi(body);
      strcpy(obody, msg[menow]); //message for now
      snprintf(contenttype, sizeof(contenttype), "Content-Type: text/plain");
    } else if(!strcmp(path, "/post")) {
      snprintf(contenttype, sizeof(contenttype), "Content-Type: application/json");
      int me = getjson(body);
      writebody(me, obody);
    } else {
      ok = 0;
      snprintf(status, sizeof(status), "HTTP/1.0 404 Not Found");
    }
  } else if(!strcmp(cmd, "HEAD")) {
    contenttype[0] = 0;
  } else {
    snprintf(status, sizeof(status), "HTTP/1.0 405 Method Not Allowed");
    contenttype[0] = 0;
    ok = 0;
  }
  snprintf(dlen, sizeof(dlen), "Content-Length: %d", (int)strlen(obody));
  j = snprintf(obuf, MAXLINE, "%s\r\n", status);
  if(ok) {
    if(contenttype[0]) j += snprintf(obuf+j, MAXLINE - j, "%s\r\n", contenttype);
    j += snprintf(obuf+j, MAXLINE - j, "%s\r\n", date);
    j += snprintf(obuf+j, MAXLINE - j, "%s\r\n\r\n", dlen);
    j += snprintf(obuf+j, MAXLINE - j, "%s", obody); //return response body
  }
}

void *count(void *arg) { //count down for starting
  pthread_detach (pthread_self());
  int me = *(int*)arg;
  sleep(1);
  msg[me][1] = msg[me + 1][1] = 0;
  msg[me][0] = msg[me + 1][0] = '3';
  sleep(1);
  msg[me][0] = msg[me + 1][0] = '2';
  sleep(1);
  msg[me][0] = msg[me + 1][0] = '1';
  sleep(1);
  msg[me][0] = msg[me + 1][0] = '0';
  pthread_exit((void *)0);
}

int getjson(char *body) { //reading json-type content (only for this file)
  char map[22][10], key[100], cont[100];
  int me = -1, end, i, j, xnow = 0, ynow = 0, pos = 0, re = 0;
  for(i = 0; i < 22; ++i) for(j = 0; j < 10; ++j) map[i][j] = 0;
  while(body[pos] == '{') ++pos;
  while(strlen(body) > pos) {
    re = 0;
    for(; pos < strlen(body); ++pos) {
      if(body[pos] == ' ') continue;
      if(body[pos] == ':') {
        ++pos;
        break;
      }
      key[re++] = body[pos]; //key
    }
    key[re] = 0;
    while(body[pos] == ' ' || body[pos] == '\r' || body[pos] == '\n') ++pos; 
    if(!strcmp(key, "\"me\"")) { //then integer
      re = 0;
      for(; pos < strlen(body); ++pos) {
        if(body[pos] == ' ') continue;
        if(body[pos] == ',') {
          ++pos;
          break;
        }
        cont[re++] = body[pos];
      }
      cont[re] = 0;
      me = atoi(cont);
    } else if(!strcmp(key, "\"map\"")) { //then 2D integer array 
        re = 0;
	for(; pos < strlen(body); ++pos) {
        if(body[pos] == ' ' || body[pos] == '[' || body[pos] == '\n' || body[pos] == '\r')
          continue; //starting charcter or splitting character
        else if(body[pos] == ',') {
          cont[re] = 0;
          map[xnow][ynow] = atoi(cont);
          ++ynow, re = 0;
        } else if(body[pos] == ']') {
          cont[re] = 0;
          map[xnow][ynow] = atoi(cont);
          ++xnow, ynow = 0, re = 0, ++pos;
          while(body[pos] == ' ' || body[pos] == ',' || body[pos] == '\n' || body[pos] == '\r') ++pos;
          if(body[pos] == ']') {
            pos++; //then reading "]]" = last character to read here
            break;
          }
        } else {
          cont[re++] = body[pos];
        }
      }
    } else if(!strcmp(key, "\"end\"")) { //then integer
      re = 0;
      for(; pos < strlen(body); ++pos) {
        if(body[pos] == ' ') continue;
        if(body[pos] == ',') {
          ++pos;
          break;
        }
        cont[re++] = body[pos];
      }
      cont[re] = 0;
      end = atoi(cont);
    }
    while(body[pos] == ' ' || body[pos] == '\r' || body[pos] == '\n' || body[pos] == ',' || body[pos] == '}')
      ++pos; //splitting character
  }
  if(end) flg[me] = 3; //losing
  if(me >= 0) for(i = 0; i < 22; ++i) for(j = 0; j < 10; ++j) block[me][i][j] = map[i][j]; //updating the arrangement
  return me;
}

void writebody(int me, char *body) { //retern json-type body
  int i, j, k = snprintf(body, MAXLINE, "{\"me\":%d,\"map\":[", me);
  int tmp = me^1;
  for(i = 0; i < 22; ++i) {
    if(i) k += snprintf(body+k, MAXLINE-k, ",");
    k += snprintf(body+k, MAXLINE-k, "[%d", block[tmp][i][0]);
    for(j = 1; j < 10; ++j) k += snprintf(body+k, MAXLINE-k, ",%d", block[tmp][i][j]);
    k += snprintf(body+k, MAXLINE-k, "]");
  }
  k += snprintf(body+k, MAXLINE-k, "],\"end\":%d}", flg[tmp] == 3 ? 1 : 0); //end: 0 -> playing, 1 -> finished
}

int main(int argc, char **argv) {
  int listen_sock, accept_sock, pid, yes, i;
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
  for(i = 0; i < MAXNOW; ++i) flg[i] = 0, snprintf(msg[i], sizeof(msg[i]), "waiting");
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
    if(pthread_create(&thread_id, NULL, send_recv_thread, (void *)&accept_sock) != 0) {
      perror("pthread_create");
    }
  }
  close(listen_sock);
  exit(0);
}
