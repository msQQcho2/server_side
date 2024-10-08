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
#include <iostream>

#define MAXLINE 60000
#define LISTENQ 1024
#define SERV_PORT 22608
#define MAXNOW 1000

using namespace std;

int now = 0;
char flg[MAXNOW], msg[MAXNOW][20], block[MAXNOW][22][10];

void oper (char *inbuf);
int getjson(char *body);
void writebody(int me, char *body);

void oper(char *inbuf) {
  if(strlen(inbuf) < 2) return;
  char cmd[10], path[100], httpv[20], body[MAXLINE + 1], obody[MAXLINE + 1] = "";
  int i, j;
  for(i = 0; inbuf[i] != ' ' && i < strlen(inbuf); ++i) cmd[i] = inbuf[i];
  cmd[i] = 0;
  int pos = ++i;
  for(i = pos; inbuf[i] != ' ' && i < strlen(inbuf); ++i) path[i - pos] = inbuf[i];
  path[i - pos] = 0;
  pos = ++i;
  for(i = pos; inbuf[i] != '\r' && inbuf[i] != '\n' && i < strlen(inbuf); ++i) httpv[i - pos] = inbuf[i];
  httpv[i - pos] = 0;
  while(inbuf[i] == '\r' || inbuf[i] == '\n') ++i;
  pos = i;
  vector<pair<string, string> > vec;
  while(pos < strlen(inbuf)) {
    char ca[100], cb[1000];
    for(i = pos; inbuf[i] != ' ' && inbuf[i] != '\r' && inbuf[i] != '\n'; ++i) {
      ca[i - pos] = inbuf[i];
    }
    if(i == pos) break;
    ca[i - pos] = 0;
    pos = ++i;
    for(; inbuf[i] != '\r' && inbuf[i] != '\n'; ++i) {
      cb[i - pos] = inbuf[i];
    }
    cb[i - pos] = 0;
    string sa = ca, sb = cb;
    vec.push_back(make_pair(sa, sb));
    pos = i + 2;
  }
  while(inbuf[i] == '\r' || inbuf[i] == '\n') ++i;
  sort(vec.begin(), vec.end());
  pos = i;
  for(i = pos; i < strlen(inbuf); ++i) body[i - pos] = inbuf[i];
  body[i - pos] = 0;
  printf("cmd: %s\npath: %shttpv: %s\n", cmd, path, httpv);
  printf("vec:\n");
  for(i = 0; i < vec.size(); ++i) cout << vec[i].first << " " << vec[i].second << endl;
  printf("\nbody: %s\n", body);
  int me = getjson(body);
}

int getjson(char *body) {
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
      key[re++] = body[pos];
    }
    key[re] = 0;
    while(body[pos] == ' ' || body[pos] == '\r' || body[pos] == '\n') ++pos;
    printf("%s\nnow: %c\n", key, body[pos]);
    if(!strcmp(key, "\"me\"")) {
      printf("me\n");
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
    } else if(!strcmp(key, "\"map\"")) {
      re = 0;
      printf("map\n{");
      for(; pos < strlen(body); ++pos) {
        if(body[pos] == ' ' || body[pos] == '[' || body[pos] == '\n' || body[pos] == '\r')
          continue;
        else if(body[pos] == ',') {
          cont[re] = 0;
	  printf("(%d %d %s %d)", xnow, ynow, cont, atoi(cont));
          map[xnow][ynow] = atoi(cont);
          ++ynow, re = 0;
        } else if(body[pos] == ']') {
          cont[re] = 0;
	  printf("(%d %d %s %d)",xnow, ynow, cont, atoi(cont));
          map[xnow][ynow] = atoi(cont);
          ++xnow, ynow = 0, re = 0, ++pos;
          while(body[pos] == ' ' || body[pos] == ',' || body[pos] == '\n' || body[pos] == '\r') ++pos;
          if(body[pos] == ']') {
            pos++;
            break;
          }
        } else {
          cont[re++] = body[pos];
        }
      }
      printf("}\n");
    } else if(!strcmp(key, "\"end\"")) {
      printf("end\n");
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
      ++pos;
  }
  if(me >= 0) for(i = 0; i < 22; ++i) {
    for(j = 0; j < 10; ++j) block[me][i][j] = map[i][j], printf("%d ", map[i][j]);
    printf("\n");
  }
  return me;
}

void writebody(int me, char *body) {
  int i, j, k = snprintf(body, MAXLINE, "{\"me\":%d,\"map\":[", me);
  int tmp = me^1;
  for(i = 0; i < 22; ++i) {
    if(i) k += snprintf(body+k, MAXLINE-k, ",");
    k += snprintf(body+k, MAXLINE-k, "[%d", block[tmp][i][0]);
    for(j = 1; j < 10; ++j) k += snprintf(body+k, MAXLINE-k, "%d,", block[tmp][i][j]);
    k += snprintf(body+k, MAXLINE-k, "]");
  }
  k += snprintf(body+k, MAXLINE-k, "],\"end\":%d}", flg[tmp] == 3 ? 1 : 0);
}

int main(int argc, char **argv) {
  char inbuf[10000] = "POST /start HTTP/1.1\r\nHost: login2.sekiya-lab.info:22608\r\nConnection: keep-alive\r\nContent-Length: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36 Edg/126.0.0.0\r\nContent-Type: text/plain;charset=UTF-8\r\nAccept: */*\r\nOrigin: http://login2.sekiya-lab.info:22608\r\nReferer: http://login2.sekiya-lab.info:22608/\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: ja,en;q=0.9,en-GB;q=0.8,en-US;q=0.7\r\n\r\n{\"map\":[[0,0,0,0,0,7,0,0,0,0],[0,0,0,0,7,7,7,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0]],\"me\":0,\"end\":0}";
  printf("%s\n", inbuf);
  oper(inbuf);
  return 0;
}
