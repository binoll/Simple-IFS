#include "blocks_bitmap.h"
#include <string.h>
#include "../debug/debug.h"

void get_block_bitmap_offset(uint32_t block_idx,
                                          uint32_t* byte_offset,
                                          uint8_t* bit_offset) {
  *byte_offset = block_idx / 8;
  *bit_offset = block_idx % 8;

  sifs_debug("Смещение блока %u: байт=%u, бит=%u\n",
             block_idx, *byte_offset, *bit_offset);
}

void block_bitmap_init(struct superblock* sb, uint8_t* bitmap) {
  sifs_debug("Инициализация битовой карты блоков\n");

  // Расчет размера битмапа
  uint32_t bitmap_size = sb->count_block_bitmap_blocks * sb->block_size;
  memset(bitmap, 0, bitmap_size);
  sifs_debug("Битовая карта обнулена (%u байт)\n", bitmap_size);

  // Пометить системные блоки как занятые
  uint32_t meta_blocks[] = {
    0,  // Суперблок
    sb->first_inode_bitmap_block,
    sb->first_block_bitmap_block,
    sb->first_inode_table_block
  };
  uint32_t meta_blocks_count[] = {
    1,  // Суперблок
    sb->count_inode_bitmap_blocks,
    sb->count_block_bitmap_blocks,
    sb->count_inode_table_blocks
  };

  // Обработка всех системных областей
  for (uint8_t i = 0; i < 4; i++) {
    uint32_t start = meta_blocks[i];
    uint32_t count = meta_blocks_count[i];

    for (uint32_t j = 0; j < count; j++) {
      uint32_t block_idx = start + j;
      if (block_idx >= sb->count_blocks) break;

      uint32_t byte_offset;
      uint8_t bit_offset;
      get_block_bitmap_offset(block_idx, &byte_offset, &bit_offset);
      bitmap[byte_offset] |= (1 << bit_offset);
      sifs_debug("Системный блок %u помечен как занятый\n", block_idx);
    }
  }

  // Обновление счетчика свободных блоков
  uint32_t total_meta_blocks =
      1 +
      sb->count_inode_bitmap_blocks +
      sb->count_block_bitmap_blocks +
      sb->count_inode_table_blocks;

  sb->count_free_blocks = sb->count_blocks - total_meta_blocks;
  sifs_debug("Системных блоков: %u, свободных блоков: %u\n",
             total_meta_blocks, sb->count_free_blocks);
}

bool is_block_allocated(const struct superblock* sb,
                        const uint8_t* bitmap,
                        uint32_t block_idx) {
  if (block_idx >= sb->count_blocks) {
    sifs_debug("Недопустимый блок: %u (всего %u блоков)\n",
               block_idx, sb->count_blocks);
    return true;
  }

  uint32_t byte_offset;
  uint8_t bit_offset;
  get_block_bitmap_offset(block_idx, &byte_offset, &bit_offset);

  bool allocated = (bitmap[byte_offset] >> bit_offset) & 1;
  sifs_debug("Блок %u: %s\n", block_idx, allocated ? "занят" : "свободен");
  return allocated;
}

int64_t allocate_block(struct superblock* sb, uint8_t* bitmap) {
  sifs_debug("Поиск свободного блока\n");

  // Начинаем поиск с первого блока данных
  for (uint32_t block_idx = sb->first_block_data;
       block_idx < sb->count_blocks;
       block_idx++) {

    if (!is_block_allocated(sb, bitmap, block_idx)) {
      // Пометить блок как занятый
      uint32_t byte_offset;
      uint8_t bit_offset;
      get_block_bitmap_offset(block_idx, &byte_offset, &bit_offset);
      bitmap[byte_offset] |= (1 << bit_offset);

      // Обновить счетчик
      sb->count_free_blocks--;

      sifs_debug("Выделен блок %u\n", block_idx);
      sifs_debug("Осталось свободных блоков: %u\n", sb->count_free_blocks);
      return block_idx;
    }
  }

  sifs_debug("Свободные блоки отсутствуют!\n");
  return -1;
}

void free_block(struct superblock* sb,
                uint8_t* bitmap,
                uint32_t block_idx) {
  // Проверка валидности
  if (block_idx < sb->first_block_data || block_idx >= sb->count_blocks) {
    sifs_debug("Недопустимый блок для освобождения: %u\n", block_idx);
    return;
  }

  if (!is_block_allocated(sb, bitmap, block_idx)) {
    sifs_debug("Блок %u уже свободен\n", block_idx);
    return;
  }

  // Снять отметку
  uint32_t byte_offset;
  uint8_t bit_offset;
  get_block_bitmap_offset(block_idx, &byte_offset, &bit_offset);
  bitmap[byte_offset] &= ~(1 << bit_offset);

  // Обновить счетчик
  sb->count_free_blocks++;

  sifs_debug("Освобожден блок %u\n", block_idx);
  sifs_debug("Новое количество свободных: %u\n", sb->count_free_blocks);
}

uint32_t count_free_blocks(const struct superblock* sb, const uint8_t* bitmap) {
  sifs_debug("Подсчет свободных блоков\n");

  uint32_t count = 0;
  const uint32_t total_bytes = (sb->count_blocks + 7) / 8;

  // Lookup-таблица для подсчета нулевых битов
  static const uint8_t lookup_table[256] = {
    8,7,7,6,7,6,6,5,7,6,6,5,6,5,5,4,7,6,6,5,6,5,5,4,6,5,5,4,5,4,4,3,
    7,6,6,5,6,5,5,4,6,5,5,4,5,4,4,3,6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
    7,6,6,5,6,5,5,4,6,5,5,4,5,4,4,3,6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
    6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,4,3,3,2,3,2,2,1,
    7,6,6,5,6,5,5,4,6,5,5,4,5,4,4,3,6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
    6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,4,3,3,2,3,2,2,1,
    6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,4,3,3,2,3,2,2,1,
    5,4,4,3,4,3,3,2,4,3,3,2,3,2,2,1,4,3,3,2,3,2,2,1,3,2,2,1,2,1,1,0
  };

  // Подсчет свободных блоков во всей карте
  for (uint32_t i = 0; i < total_bytes; i++) {
    count += lookup_table[bitmap[i]];
  }

  // Корректировка для последнего байта
  if (sb->count_blocks % 8 != 0) {
    uint8_t mask = (1 << (sb->count_blocks % 8)) - 1;
    count -= lookup_table[bitmap[total_bytes - 1] | ~mask];
  }

  // Вычитаем системные блоки (они не входят в область данных)
  uint32_t meta_blocks =
      1 +
      sb->count_inode_bitmap_blocks +
      sb->count_block_bitmap_blocks +
      sb->count_inode_table_blocks;

  count -= meta_blocks;

  // Проверка согласованности
  if (count != sb->count_free_blocks) {
    sifs_debug("Расхождение! Подсчет: %u, суперблок: %u\n",
               count, sb->count_free_blocks);
  } else {
    sifs_debug("Свободных блоков: %u (совпадает с суперблоком)\n", count);
  }

  return count;
}
