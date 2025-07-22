#pragma once

#include <stdint.h>
#include <time.h>
#include "../inode_table/inode.h"

#define FS_MAGIC 0x53494653                         // Магическое число ФС (SIFS)
#define FS_NAME "SIFS v1.0"                         // Название файловой системы
#define MAX_FS_NAME 32                              // Максимальная длина имени ФС
#define DEFAULT_BLOCK_SIZE 512                      // Стандартный размер блока (2 КБ)
#define DEFAULT_INODE_SIZE sizeof(struct inode)     // Размер inode по умолчанию
#define INODES_PER_BLOCK(block_size, inode_size) \
    ((block_size) / (inode_size))                   // Расчет максимального количества inode в блоке

// Структура суперблока
struct superblock {
    // Идентификация
    uint32_t magic;                                 // Магическое число
    char fs_name[MAX_FS_NAME];                      // Имя ФС

    // Геометрия
    uint32_t block_size;                            // Размер блока в байтах
    uint32_t inode_size;                            // Размер inode в байтах

    // Расположение структур
    uint32_t first_inode_bitmap_block;              // Стартовый блок битмапа inode
    uint32_t first_block_bitmap_block;              // Стартовый блок битмапа блоков
    uint32_t first_inode_table_block;               // Стартовый блок таблицы inode
    uint32_t first_block_data;                      // Стартовый блок области данных

    // Ресурсы
    uint32_t count_inodes;                          // Общее количество inode
    uint32_t count_free_inodes;                     // Количество свободных inode
    uint32_t count_blocks;                          // Общее количество блоков
    uint32_t count_free_blocks;                     // Количество свободных блоков

    // Размеры областей
    uint32_t count_inode_bitmap_blocks;             // Блоков под битмап inode
    uint32_t count_block_bitmap_blocks;             // Блоков под битмап блоков
    uint32_t count_inode_table_blocks;              // Блоков под таблицу inode
    uint32_t count_blocks_data;                     // Блоков данных

    // Корневой каталог
    uint32_t root_inode;                            // Inode корневого каталога

    // Состояние
    time_t last_mount;                              // Время последнего монтирования
    uint8_t clean_shutdown;                         // Флаг корректного завершения (1 = да)
};

// Проверка валидности суперблока по магическому числу
extern uint8_t superblock_valid(const struct superblock* sb);

// Расчет блоков для хранения битовой карты
extern uint32_t get_bitmap_blocks(uint32_t bits, uint32_t block_size);

// Инициализация суперблока для нового раздела
extern void init_superblock(struct superblock* sb, uint32_t space_size);
