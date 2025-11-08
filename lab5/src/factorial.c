#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>

typedef struct {
    int start;
    int end;
    long long mod;
    long long partial_result;
} thread_data_t;

// Глобальные переменные для синхронизации
long long global_result = 1;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция для вычисления частичного факториала
void* partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    data->partial_result = 1;
    
    // Вычисляем частичный факториал
    for (int i = data->start; i <= data->end; i++) {
        data->partial_result = (data->partial_result * i) % data->mod;
    }
    
    printf("Поток: факториал от %d до %d = %lld (mod %lld)\n", 
           data->start, data->end, data->partial_result, data->mod);
    
    // СИНХРОНИЗИРОВАННОЕ обновление глобального результата
    pthread_mutex_lock(&result_mutex);
    global_result = (global_result * data->partial_result) % data->mod;
    printf("Обновление глобального результата: %lld\n", global_result);
    pthread_mutex_unlock(&result_mutex);
    
    pthread_exit(NULL);
}

// Функция для парсинга аргументов командной строки
void parse_arguments(int argc, char* argv[], int* k, int* pnum, long long* mod) {
    // Значения по умолчанию
    *k = 10;
    *pnum = 4;
    *mod = 1000000007;
    
    static struct option long_options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "k:p:m:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'k':
                *k = atoi(optarg);
                break;
            case 'p':
                *pnum = atoi(optarg);
                break;
            case 'm':
                *mod = atoll(optarg);
                break;
            default:
                fprintf(stderr, "Использование: %s -k <число> --pnum=<потоки> --mod=<модуль>\n", argv[0]);
                exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    int k, pnum;
    long long mod;
    
    // Парсинг аргументов
    parse_arguments(argc, argv, &k, &pnum, &mod);
    
    printf("Вычисление %d! mod %lld using %d потоков\n", k, mod, pnum);
    
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Ошибка: все параметры должны быть положительными числами\n");
        return 1;
    }
    
    // Если потоков больше чем чисел для вычисления
    if (pnum > k) {
        pnum = k;
        printf("Предупреждение: уменьшено количество потоков до %d\n", pnum);
    }
    
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
    // Распределение работы между потоками
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    // Инициализация мьютекса
    pthread_mutex_init(&result_mutex, NULL);
    
    // Создание потоков
    for (int i = 0; i < pnum; i++) {
        int numbers_for_this_thread = numbers_per_thread;
        if (i < remainder) {
            numbers_for_this_thread++;
        }
        
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + numbers_for_this_thread - 1;
        thread_data[i].mod = mod;
        
        printf("Поток %d: числа от %d до %d\n", 
               i, thread_data[i].start, thread_data[i].end);
        
        if (pthread_create(&threads[i], NULL, partial_factorial, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            return 1;
        }
        
        current_start += numbers_for_this_thread;
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка ожидания потока");
            return 1;
        }
    }
    
    // Уничтожение мьютекса
    pthread_mutex_destroy(&result_mutex);
    
    printf("\nФинальный результат: %d! mod %lld = %lld\n", k, mod, global_result);
    
    // Проверка (последовательное вычисление для верификации)
    long long sequential_result = 1;
    for (int i = 1; i <= k; i++) {
        sequential_result = (sequential_result * i) % mod;
    }
    printf("Проверка (последовательно): %lld\n", sequential_result);
    
    // Проверка корректности
    if (global_result == sequential_result) {
        printf("✓ Результаты совпадают!\n");
    } else {
        printf("✗ Ошибка: результаты не совпадают!\n");
    }
    
    return 0;
}