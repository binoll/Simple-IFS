#pragma once

#include "../superblock/superblock.h"
#include <stdbool.h>

// Рассчитывает смещение в битовой карте блоков
extern void get_block_bitmap_offset(uint32_t block_idx,
                                          uint32_t* byte_offset,
                                          uint8_t* bit_offset);

// Инициализирует битовую карту блоков (помечает системные блоки)
extern void block_bitmap_init(struct superblock* sb, uint8_t* bitmap);

// Проверяет, занят ли указанный блок данных
extern bool is_block_allocated(const struct superblock* sb,
                        const uint8_t* bitmap,
                        uint32_t block_idx);

// Выделяет свободный блок и возвращает его индекс
extern int64_t allocate_block(struct superblock* sb, uint8_t* bitmap);

// Освобождает указанный блок
extern void free_block(struct superblock* sb, uint8_t* bitmap, uint32_t block_idx);

// Подсчитывает количество свободных блоков данных
extern uint32_t count_free_blocks(const struct superblock* sb, const uint8_t* bitmap);
