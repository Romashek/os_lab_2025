#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

volatile sig_atomic_t timeout_occurred = 0;

void timeout_handler(int sig) {
    timeout_occurred = 1;
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = 0; // 0 означает отсутствие таймаута
    bool with_files = false;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"timeout", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 3:
                        timeout = atoi(optarg);
                        if (timeout <= 0) {
                            printf("timeout must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 4:
                        with_files = true;
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"]\n",
               argv[0]);
        return 1;
    }

    // Настройка обработчика сигнала таймаута
    if (timeout > 0) {
        struct sigaction sa;
        sa.sa_handler = timeout_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGALRM, &sa, NULL);
        
        // Установка таймера
        alarm(timeout);
        printf("Timeout set to %d seconds\n", timeout);
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    int active_child_processes = 0;

    // Сохраняем PID дочерних процессов для возможного завершения
    pid_t *child_pids = malloc(pnum * sizeof(pid_t));

    int **pipes = NULL;
    if (!with_files) {
        pipes = malloc(pnum * sizeof(int*));
        for (int i = 0; i < pnum; i++) {
            pipes[i] = malloc(2 * sizeof(int));
            if (pipe(pipes[i]) == -1) {
                printf("Pipe creation failed!\n");
                return 1;
            }
        }
    }

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Создание дочерних процессов
    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            active_child_processes += 1;
            if (child_pid == 0) {
                // Дочерний процесс
                int segment_size = array_size / pnum;
                int begin = i * segment_size;
                int end = (i == pnum - 1) ? array_size : (i + 1) * segment_size;

                struct MinMax local_min_max = GetMinMax(array, begin, end);

                if (with_files) {
                    char filename_min[32], filename_max[32];
                    sprintf(filename_min, "min_%d.txt", i);
                    sprintf(filename_max, "max_%d.txt", i);
                    
                    FILE *file_min = fopen(filename_min, "w");
                    FILE *file_max = fopen(filename_max, "w");
                    if (file_min && file_max) {
                        fprintf(file_min, "%d", local_min_max.min);
                        fprintf(file_max, "%d", local_min_max.max);
                        fclose(file_min);
                        fclose(file_max);
                    }
                } else {
                    close(pipes[i][0]);
                    write(pipes[i][1], &local_min_max.min, sizeof(int));
                    write(pipes[i][1], &local_min_max.max, sizeof(int));
                    close(pipes[i][1]);
                }
                free(array);
                if (!with_files) {
                    for (int j = 0; j < pnum; j++) {
                        free(pipes[j]);
                    }
                    free(pipes);
                }
                free(child_pids);
                exit(0);
            } else {
                // Родительский процесс - сохраняем PID дочернего
                child_pids[i] = child_pid;
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    // Закрываем ненужные концы pipe в родительском процессе
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            close(pipes[i][1]);
        }
    }

    // Ожидание завершения дочерних процессов с проверкой таймаута
    while (active_child_processes > 0) {
        if (timeout_occurred) {
            printf("Timeout occurred! Sending SIGKILL to child processes...\n");
            for (int i = 0; i < pnum; i++) {
                if (child_pids[i] > 0) {
                    kill(child_pids[i], SIGKILL);
                    printf("Sent SIGKILL to child process %d\n", child_pids[i]);
                }
            }
            break;
        }
        
        pid_t finished_pid = waitpid(-1, NULL, WNOHANG);
        if (finished_pid > 0) {
            active_child_processes -= 1;
            // Отмечаем завершенный процесс
            for (int i = 0; i < pnum; i++) {
                if (child_pids[i] == finished_pid) {
                    child_pids[i] = 0; // Помечаем как завершенный
                    break;
                }
            }
        } else if (finished_pid == 0) {
            // Есть еще работающие процессы, ждем немного
            usleep(100000); // 100ms
        } else {
            // Ошибка waitpid
            perror("waitpid");
            break;
        }
    }

    // Если был таймаут, дожидаемся завершения убитых процессов
    if (timeout_occurred) {
        printf("Waiting for killed processes to terminate...\n");
        while (active_child_processes > 0) {
            waitpid(-1, NULL, 0);
            active_child_processes -= 1;
        }
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    // Сбор результатов только от процессов, которые завершились нормально
    for (int i = 0; i < pnum; i++) {
        // Если процесс был убит по таймауту, пропускаем его результаты
        if (timeout_occurred && child_pids[i] != 0) {
            printf("Skipping results from killed process %d\n", child_pids[i]);
            continue;
        }

        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {
            char filename_min[32], filename_max[32];
            sprintf(filename_min, "min_%d.txt", i);
            sprintf(filename_max, "max_%d.txt", i);
            
            FILE *file_min = fopen(filename_min, "r");
            FILE *file_max = fopen(filename_max, "r");
            if (file_min && file_max) {
                fscanf(file_min, "%d", &min);
                fscanf(file_max, "%d", &max);
                fclose(file_min);
                fclose(file_max);
            }
        } else {
            read(pipes[i][0], &min, sizeof(int));
            read(pipes[i][0], &max, sizeof(int));
            close(pipes[i][0]);
        }

        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            free(pipes[i]);
        }
        free(pipes);
    }

    if (timeout_occurred) {
        printf("Calculation interrupted by timeout after %d seconds\n", timeout);
        printf("Partial results:\n");
    }
    
    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}