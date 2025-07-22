#pragma once

#include "inode.h"
#include "../superblock/superblock.h"
#include <stdbool.h>

// Инициализирует таблицу inode (вызывается при создании ФС)
extern void init_inode_table(struct superblock* sb, void* table);

// Читает inode из таблицы по индексу
extern bool read_inode(struct superblock* sb,
                       void* table,
                       uint32_t inode_idx,
                       struct inode* node);

// Записывает inode в таблицу по индексу
extern bool write_inode(struct superblock* sb,
                        void* table,
                        uint32_t inode_idx,
                        const struct inode* node);

// Рассчитывает позицию inode в таблице
extern void get_inode_position(struct superblock* sb,
                                             uint32_t inode_idx,
                                             uint32_t* block_offset,
                                             uint32_t* byte_offset);
