/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

// 프로그램은 호스트 주소와 포트 번호를 인자로 받는다.
int main(int argc, char **argv)
{
  int clientfd;                    // 서버와 연결될 소캣 식별자가 담긴다.
  char *host, *port, buf[MAXLINE]; // 연결할 서버의 host, port, 입출력 버퍼가 담긴다.
  rio_t rio;                       // Robust I/O를 위한 구조체(커널 버퍼 + 사용자 버퍼 사이 관리)

  if (argc != 3) // 요구하는 인자를 다 받지 못했을 경우 에러 처리
  {
    fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    exit(0);
  }
  host = argv[1]; // 첫 번째 인자는 연결하는 서버의 host IP 주소(혹은 도메인)를 의미한다.
  port = argv[2]; // 두 번째 인자는 연결하는 서버의 포트를 의미한다.

  // socket(), getaddrinfo(), connect() 과정이 포함된다.
  clientfd = Open_clientfd(host, port); // 해당 서버와 연결할 클라이언트 포트를 생성/연결한다.
  Rio_readinitb(&rio, clientfd);        // 연결된 상태의 소켓과 입출력 구조체를 초기화하여 사용할 수 있도록 한다.

  while (Fgets(buf, MAXLINE, stdin) != NULL) // 한 줄씩 입력
  {
    Rio_writen(clientfd, buf, strlen(buf)); // 입력한 문자열을 서버로 전송한다.
    Rio_readlineb(&rio, buf, MAXLINE);      // 서버로부터 응답을 받는다.
    Fputs(buf, stdout);                     // 모니터(터미널)에 띄운다.
  }
  Close(clientfd); // line:netp:echoclient:close // 연결 종료
  exit(0);
}
/* $end echoclientmain */