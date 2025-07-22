#pragma once

#include <stdint.h>
#include <unistd.h>

// Открыть образ SIFS
extern int32_t image_open(const char* filename, int32_t truncate);

// Закрыть образ SIFS
extern int32_t image_close(void);

// Чтение образ SIFS
extern ssize_t image_read(void* buffer, size_t size, off_t offset);

// Запись в образ SIFS
extern ssize_t image_write(const void* buffer, size_t size, off_t offset);
