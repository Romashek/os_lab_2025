#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>

// Структура для передачи аргументов факториала
struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

// Модульное умножение (a * b) % mod
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

// Конвертация строки в uint64_t с проверкой ошибок
bool ConvertStringToUI64(const char *str, uint64_t *val);

#endif // COMMON_H