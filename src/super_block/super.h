#pragma once

#include <stdint.h>
#include <time.h>

#define FS_MAGIC 0x53494653 // "SIFS" - Simple Inode File System
#define FS_NAME "SIFS v1.0"
#define MAX_FS_NAME 32
#define DEFAULT_BLOCK_SIZE 1024
#define DEFAULT_INODE_SIZE 128
#define SIFS_DEBUG

struct superblock {
  // Идентификация
  uint32_t magic; // Всегда SIFS_MAGIC
  char fs_name[MAX_FS_NAME]; // "SIFS"

  // Геометрия
  uint32_t block_size; // Размер блока
  uint32_t inode_size; // Размер одного inode

  // Управление ресурсами
  uint32_t inode_count; // Общее количество inode
  uint32_t free_inodes; // Общее количество свободных inode
  uint32_t block_count; // Общее количество блоков
  uint32_t free_blocks; // Общее количество свободных блоков

  // Расположение ключевых структур
  uint32_t inode_bitmap_block; // Стартовый блок битмапа inode
  uint32_t block_bitmap_block; // Стартовый блок битмапа блоков
  uint32_t inode_table_block; // Стартовый блок таблицы inode
  uint32_t first_data_block; // Первый блок данных

  // Корневой каталог
  uint32_t root_inode; // Inode корневого каталога (всегда 1)

  // Состояние
  time_t last_mount; // Время последнего монтирования
  uint8_t clean_shutdown; // Флаг корректного размонтирования
};

// Валидация суперблока
extern static inline uint8_t sb_valid(const struct superblock* sb);

// Расчет блоков для битовой карты
extern static inline uint32_t bitmap_blocks(uint32_t bits, uint32_t block_size);

// Инициализация суперблока
extern static inline void init_superblock(struct superblock* sb,
                                          uint32_t total_blocks,
                                          uint32_t inode_count);