#pragma once

// Глобальная отладка
#define SIFS_DEBUG

// Определяем, компилируем ли для ядра Linux
#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/bug.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef SIFS_DEBUG
#ifdef __KERNEL__
// Реализация для ядра Linux
#define sifs_debug(fmt, ...) \
do { \
  printk(KERN_DEBUG "SIFS DEBUG (%s:%d %s): " fmt, __FILE__, __LINE__, \
    __func__, ##__VA_ARGS__); \
} while (0)
#define sifs_assert(expr) \
do { \
  if (!(expr)) { \
    printk(KERN_EMERG "SIFS ASSERT FAILED (%s:%d %s): %s\n", __FILE__, \
      __LINE__, __func__, #expr); \
    BUG(); \
  } \
} while (0)
#else
// Реализация для пользовательского пространства
#define sifs_debug(fmt, ...) \
do { \
  fprintf(stderr, "SIFS DEBUG (%s:%d %s): " fmt, __FILE__, __LINE__, __func__, \
    ##__VA_ARGS__); \
} while (0)
#define sifs_assert(expr) \
do { \
  if (!(expr)) { \
    fprintf(stderr, "SIFS ASSERT FAILED (%s:%d %s): %s\n", \
      __FILE__, __LINE__, __func__, #expr); \
    abort(); \
  } \
} while (0)
#endif
#else
// Пустые макросы, если отладка отключена
#define sifs_debug(fmt, ...)  ((void)0)
#define sifs_assert(expr)     ((void)0)
#endif