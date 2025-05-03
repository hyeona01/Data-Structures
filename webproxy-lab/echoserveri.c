/*
 * echoserveri.c - An iterative echo server
 */
/* $begin echoserverimain */
#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv)
{
  int listenfd, connfd; // 대기용 소캣 식별자, 클라이언트와 연결이 완료된 소캣 식별자 각각 선언
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;                  /* Enough space for any address */
  char client_hostname[MAXLINE], client_port[MAXLINE]; // 연결될 클라이언트의 host(IP 혹은 도메인), port를 담는다.

  if (argc != 2) // 인자료 포트 번호를 요구한다.
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  // getaddrinfo(), socket(), bind(), listen() 과정이 포함된다.
  listenfd = Open_listenfd(argv[1]); // 서버의 소켓 디스크립터를 만들고 + 주소 바인딩 + 클라이언트의 요청을 기다린다.
  while (1)
  {
    clientlen = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);           // 클라이언트의 연결 요청을 수락한다.(connfd→클라이언트와의 통신용 소켓)
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, // 연결된 클라이언트의 호스트 이름과 포트를 출력
                client_port, MAXLINE, 0);
    printf("Connected to (%s, %s)\n", client_hostname, client_port);
    echo(connfd);  // 클라이언트 요청에 대한 처리를 echo로 수행한다.
    Close(connfd); // 종료
  }
  exit(0);
}
/* $end echoserverimain */