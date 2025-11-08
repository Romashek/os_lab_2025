#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <inttypes.h>
#include "common.h"

struct Server {
  char ip[255];
  int port;
};

int main(int argc, char **argv) {
  uint64_t k = 0;
  uint64_t mod = 0;
  char servers_file[255] = {'\0'};

  while (true) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) {
          fprintf(stderr, "Invalid k value: %s\n", optarg);
          return 1;
        }
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) {
          fprintf(stderr, "Invalid mod value: %s\n", optarg);
          return 1;
        }
        break;
      case 2:
        // Исправление: безопасное копирование
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        servers_file[sizeof(servers_file) - 1] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == 0 || mod == 0 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // Чтение серверов из файла
  FILE *file = fopen(servers_file, "r");
  if (!file) {
    fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
    return 1;
  }

  struct Server servers[10];
  int servers_num = 0;
  char line[255];

  while (fgets(line, sizeof(line), file) && servers_num < 10) {
    char *colon = strchr(line, ':');
    if (colon) {
      *colon = '\0';
      // Исправление: безопасное копирование без предупреждения
      size_t ip_len = strlen(line);
      if (ip_len >= sizeof(servers[servers_num].ip)) {
        ip_len = sizeof(servers[servers_num].ip) - 1;
      }
      memcpy(servers[servers_num].ip, line, ip_len);
      servers[servers_num].ip[ip_len] = '\0';
      servers[servers_num].port = atoi(colon + 1);
      servers_num++;
    }
  }
  fclose(file);

  if (servers_num == 0) {
    fprintf(stderr, "No valid servers found in file\n");
    return 1;
  }

  printf("Found %d servers\n", servers_num);

  // Распределение работы между серверами
  uint64_t total_result = 1;
  int active_connections = 0;
  int sockets[servers_num];

  // Установка соединений и отправка задач
  for (int i = 0; i < servers_num; i++) {
    struct hostent *hostname = gethostbyname(servers[i].ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", servers[i].ip);
      continue;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(servers[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr_list[0]);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      continue;
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection to %s:%d failed\n", servers[i].ip, servers[i].port);
      close(sck);
      continue;
    }

    // Расчет диапазона для этого сервера
    uint64_t numbers_per_server = k / servers_num;
    uint64_t remainder = k % servers_num;
    
    uint64_t begin = i * numbers_per_server + 1;
    uint64_t end = (i + 1) * numbers_per_server;
    
    if (i == servers_num - 1) {
      end += remainder;
    }

    // Подготовка и отправка задачи
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
      fprintf(stderr, "Send to %s:%d failed\n", servers[i].ip, servers[i].port);
      close(sck);
      continue;
    }

    sockets[active_connections] = sck;
    active_connections++;
    
    printf("Sent task to %s:%d - range [%" PRIu64 ", %" PRIu64 "]\n", 
           servers[i].ip, servers[i].port, begin, end);
  }

  // Получение результатов от всех серверов
  for (int i = 0; i < active_connections; i++) {
    char response[sizeof(uint64_t)];
    if (recv(sockets[i], response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Receive from server %d failed\n", i);
      close(sockets[i]);
      continue;
    }

    uint64_t partial_result;
    memcpy(&partial_result, response, sizeof(uint64_t));
    total_result = MultModulo(total_result, partial_result, mod);
    
    printf("Received partial result: %" PRIu64 "\n", partial_result);
    close(sockets[i]);
  }

  printf("Final result: %" PRIu64 "! mod %" PRIu64 " = %" PRIu64 "\n", k, mod, total_result);

  return 0;
}