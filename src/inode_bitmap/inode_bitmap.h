#pragma once

#include "../super_block/super.h"
#include <stdbool.h>

// Инициализация битовой карты inode
void inode_bitmap_init(struct superblock* sb, uint8_t* bitmap);

// Проверка состояния inode
bool inode_allocated(struct superblock* sb, uint8_t* bitmap,
                     uint32_t inode_idx);

// Выделение inode
int allocate_inode(struct superblock* sb, uint8_t* bitmap);

// Освобождение inode
void free_inode(struct superblock* sb, uint8_t* bitmap, uint32_t inode_idx);

// Подсчет свободных inode
uint32_t count_free_inodes(struct superblock* sb, uint8_t* bitmap);