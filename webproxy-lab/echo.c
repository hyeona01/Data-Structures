/*
 * echo - read and echo text lines until client closes connection
 */
/* $begin echo */
#include "csapp.h"

void echo(int connfd) // 클라이언트와 연결 완료된 소켓 디스크립터
{
  size_t n;
  char buf[MAXLINE];
  rio_t rio; // Robust I/O를 위한 구조체(커널 버퍼 + 사용자 버퍼 사이 관리)

  Rio_readinitb(&rio, connfd);                         // connfd 소켓을 rio와 연결하여 입출력 구조체를 사용할 수 있도록 한다.
  while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) // 클라이언트로부터 받은 데이터를 한 줄씩 읽는다.
  {
    printf("server received %d bytes\n", (int)n); // 서버가 받은 데이터의 bytes 단위 사이즈
    Rio_writen(connfd, buf, n);                   // 클라이언트로 받은 버퍼와 동일한 버퍼를 response로 전송한다.
  }
}
/* $end echo */