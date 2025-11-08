#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Usage: %s <IP> <PORT> <BUFSIZE>\n", argv[0]);
    exit(1);
  }

  char *ip = argv[1];
  int port = atoi(argv[2]);
  int bufsize = atoi(argv[3]);

  // Проверка размера буфера
  if (bufsize <= 0) {
    printf("Error: BUFSIZE must be positive\n");
    exit(1);
  }

  int fd;
  int nread;
  char *buf = malloc(bufsize);  // Динамическое выделение памяти
  if (buf == NULL) {
    perror("malloc");
    exit(1);
  }
  
  struct sockaddr_in servaddr;
  
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    free(buf);
    exit(1);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("bad address");
    free(buf);
    close(fd);
    exit(1);
  }

  servaddr.sin_port = htons(port);

  if (connect(fd, (SADDR *)&servaddr, sizeof(servaddr)) < 0) {
    perror("connect");
    free(buf);
    close(fd);
    exit(1);
  }

  printf("Connected to server. Type messages (Ctrl+D to exit):\n");
  
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      break;
    }
  }

  free(buf);
  close(fd);
  printf("Disconnected\n");
  return 0;
}