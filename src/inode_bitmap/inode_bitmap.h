#pragma once

#include "../superblock/superblock.h"
#include <stdbool.h>

// Рассчитывает смещение в битовой карте для указанного inode
extern void get_bitmap_offset(uint32_t inode_idx,
                                            uint32_t* byte_offset,
                                            uint8_t* bit_offset);

// Инициализирует битовую карту inode (помечает зарезервированные inode)
extern void inode_bitmap_init(struct superblock* sb, uint8_t* bitmap);

// Проверяет, выделен ли указанный inode
extern bool is_inode_allocated(const struct superblock* sb,
                               const uint8_t* bitmap,
                               uint32_t inode_idx);

// Выделяет свободный inode и возвращает его индекс
extern int64_t allocate_inode(struct superblock* sb, uint8_t* bitmap);

// Освобождает указанный inode
extern void free_inode(struct superblock* sb, uint8_t* bitmap,
                       uint32_t inode_idx);

// Подсчитывает количество свободных inode в битовой карте
extern uint32_t count_free_inodes(const struct superblock* sb,
                                  const uint8_t* bitmap);
