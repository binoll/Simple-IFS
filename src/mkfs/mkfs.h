#pragma once

#include <stdint.h>

// Создание образа SIFS (запись в файл)
extern int32_t mkfs(const char* filename, uint32_t size);
