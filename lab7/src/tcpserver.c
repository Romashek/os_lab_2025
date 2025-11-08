#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: %s <PORT> <BUFSIZE>\n", argv[0]);
    exit(1);
  }

  int port = atoi(argv[1]);
  int bufsize = atoi(argv[2]);

  // Проверка размера буфера
  if (bufsize <= 0) {
    printf("Error: BUFSIZE must be positive\n");
    exit(1);
  }

  int lfd, cfd;
  int nread;
  char *buf = malloc(bufsize);  // Динамическое выделение памяти
  if (buf == NULL) {
    perror("malloc");
    exit(1);
  }
  
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    free(buf);
    exit(1);
  }

  // Добавляем опцию переиспользования порта
  int opt = 1;
  if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    free(buf);
    close(lfd);
    exit(1);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(lfd, (SADDR *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind");
    free(buf);
    close(lfd);
    exit(1);
  }

  if (listen(lfd, 5) < 0) {
    perror("listen");
    free(buf);
    close(lfd);
    exit(1);
  }

  printf("TCP Server listening on port %d\n", port);

  while (1) {
    unsigned int clilen = sizeof(cliaddr);

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      continue;  // Продолжаем работать при ошибке accept
    }
    
    printf("Connection established from %s:%d\n", 
           inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

    while ((nread = read(cfd, buf, bufsize)) > 0) {
      printf("Received: ");
      fwrite(buf, 1, nread, stdout);  // Безопасный вывод
      printf("\n");
    }

    if (nread == -1) {
      perror("read");
    }
    
    close(cfd);
    printf("Connection closed\n");
  }

  free(buf);
  close(lfd);
  return 0;
}