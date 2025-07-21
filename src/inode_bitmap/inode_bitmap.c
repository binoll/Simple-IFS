#include "inode_bitmap.h"
#include <string.h>
#include "../debug/debug.h"

static inline void get_bitmap_offset(uint32_t inode_idx,
                                     uint32_t* byte_offset,
                                     uint8_t* bit_offset) {
  // Байтовое смещение = индекс inode / 8 (целочисленное деление)
  *byte_offset = inode_idx / 8;

  // Бит в байте = остаток от деления индекса на 8
  *bit_offset = inode_idx % 8;

  // Отладочный вывод
  sifs_debug("Смещение в битмапе: inode_idx=%u, байт=%u, бит=%u\n",
             inode_idx, *byte_offset, *bit_offset);
}

void inode_bitmap_init(struct superblock* sb, uint8_t* bitmap) {
  sifs_debug("Инициализация битовой карты inode\n");

  // Расчет размера битовой карты в блоках и байтах
  uint32_t bitmap_blocks = get_bitmap_blocks(sb->count_inodes, sb->block_size);
  uint32_t bitmap_size = bitmap_blocks * sb->block_size;

  sifs_debug("Размер битмапа: %u байт (%u блоков)\n", bitmap_size,
             bitmap_blocks);

  // Обнуление всей битовой карты (все inode свободны)
  memset(bitmap, 0, bitmap_size);
  sifs_debug("Битовая карта обнулена\n");

  // Резервирование inode 0 (не используется)
  uint32_t byte_offset;
  uint8_t bit_offset;
  get_bitmap_offset(0, &byte_offset, &bit_offset);
  bitmap[byte_offset] |= 1 << bit_offset;
  sifs_debug("Inode 0 помечен как занятый\n");

  // Резервирование корневого inode (обычно индекс 1)
  get_bitmap_offset(sb->root_inode, &byte_offset, &bit_offset);
  bitmap[byte_offset] |= 1 << bit_offset;
  sifs_debug("Корневой inode %u помечен как занятый\n", sb->root_inode);

  // Обновление счетчика свободных inode в суперблоке
  sb->count_free_inodes = sb->count_inodes - 2; // -2 (inode 0 и корневой)
  sifs_debug("Свободных inodes: %u\n", sb->count_free_inodes);
}

bool is_inode_allocated(const struct superblock* sb, const uint8_t* bitmap,
                        uint32_t inode_idx) {
  // Проверка допустимости индекса
  if (inode_idx < 1 || inode_idx >= sb->count_inodes) {
    sifs_debug("Недопустимый индекс inode: %u (максимум %u)\n", inode_idx,
               sb->count_inodes);
    return true; // Невалидные inode считаются занятыми
  }

  // Расчет позиции в битовой карте
  uint32_t byte_offset;
  uint8_t bit_offset;
  get_bitmap_offset(inode_idx, &byte_offset, &bit_offset);

  // Проверка состояния бита (0 - свободен, 1 - занят)
  bool is_allocated = bitmap[byte_offset] >> bit_offset & 1;
  sifs_debug("Состояние inode %u: %s\n",
             inode_idx, is_allocated ? "занят" : "свободен");

  return is_allocated;
}

int64_t allocate_inode(struct superblock* sb, uint8_t* bitmap) {
  sifs_debug("Поиск свободного inode\n");

  // Поиск первого свободного inode (начиная с 1)
  for (uint32_t i = 1; i < sb->count_inodes; i++) {
    if (!is_inode_allocated(sb, bitmap, i)) {
      // Расчет позиции найденного inode
      uint32_t byte_offset;
      uint8_t bit_offset;
      get_bitmap_offset(i, &byte_offset, &bit_offset);

      // Установка бита (пометка как занятый)
      bitmap[byte_offset] |= 1 << bit_offset;

      // Обновление счетчика в суперблоке
      sb->count_free_inodes--;

      sifs_debug("Выделен inode %u\n", i);
      sifs_debug("Осталось свободных: %u\n", sb->count_free_inodes);

      return i; // Возврат индекса выделенного inode
    }
  }

  sifs_debug("Свободные inodes отсутствуют\n");
  return -1; // Код ошибки
}

void free_inode(struct superblock* sb, uint8_t* bitmap, uint32_t inode_idx) {
  // Проверка валидности индекса
  if (inode_idx < 1 || inode_idx >= sb->count_inodes) {
    sifs_debug("Неверный индекс для освобождения: %u\n", inode_idx);
    return;
  }

  // Проверка, что inode действительно занят
  if (!is_inode_allocated(sb, bitmap, inode_idx)) {
    sifs_debug("Inode %u уже свободен\n", inode_idx);
    return;
  }

  // Расчет позиции inode в битовой карте
  uint32_t byte_offset;
  uint8_t bit_offset;
  get_bitmap_offset(inode_idx, &byte_offset, &bit_offset);

  // Сброс бита (установка в 0)
  bitmap[byte_offset] &= ~(1 << bit_offset);

  // Обновление счетчика
  sb->count_free_inodes++;

  sifs_debug("Освобожден inode %u\n", inode_idx);
  sifs_debug("Новое количество свободных: %u\n", sb->count_free_inodes);
}

uint32_t count_free_inodes(const struct superblock* sb, const uint8_t* bitmap) {
  sifs_debug("Подсчет свободных inodes\n");

  uint32_t count = 0;
  const uint32_t total_bytes = (sb->count_inodes + 7) / 8; // Округление вверх

  // Обработка первого байта (особый случай - пропуск inode 0)
  if (total_bytes > 0) {
    uint8_t byte = bitmap[0];
    // Проверка только битов 1-7 (inode 1-7)
    for (uint8_t bit = 1; bit < 8 && bit < sb->count_inodes; ++bit) {
      if (!(byte & 1 << bit)) count++; // Подсчет нулевых битов
    }
  }

  // Обработка полных средних байтов
  for (uint32_t byte_idx = 1; byte_idx < total_bytes - 1; ++byte_idx) {
    // Использование lookup-таблицы для быстрого подсчета нулевых битов
    static const uint8_t lookup_table[256] = {
      8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4, 7, 6, 6, 5, 6, 5, 5, 4,
      6, 5, 5, 4, 5, 4, 4, 3,
      7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 6, 5, 5, 4, 5, 4, 4, 3,
      5, 4, 4, 3, 4, 3, 3, 2,
      7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 6, 5, 5, 4, 5, 4, 4, 3,
      5, 4, 4, 3, 4, 3, 3, 2,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2,
      4, 3, 3, 2, 3, 2, 2, 1,
      7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 6, 5, 5, 4, 5, 4, 4, 3,
      5, 4, 4, 3, 4, 3, 3, 2,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2,
      4, 3, 3, 2, 3, 2, 2, 1,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2,
      4, 3, 3, 2, 3, 2, 2, 1,
      5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1, 4, 3, 3, 2, 3, 2, 2, 1,
      3, 2, 2, 1, 2, 1, 1, 0
  };
    count += lookup_table[bitmap[byte_idx]];
  }

  // Обработка последнего байта (если требуется частичная проверка)
  if (total_bytes > 1) {
    uint32_t last_byte = total_bytes - 1;
    uint8_t byte = bitmap[last_byte];
    uint8_t valid_bits = sb->count_inodes % 8;
    if (valid_bits == 0) valid_bits = 8; // Корректировка для полных байтов

    // Проверка только значимых битов
    for (uint8_t bit = 0; bit < valid_bits; ++bit) {
      if (!(byte & 1 << bit)) count++;
    }
  }

  // Проверка согласованности с суперблоком
  if (count != sb->count_free_inodes) {
    sifs_debug(
        "Обнаружено несоответствие! Подсчитано: %u, в суперблоке: %u\n",
        count, sb->count_free_inodes);
  } else {
    sifs_debug("Свободных inodes: %u (совпадает с суперблоком)\n", count);
  }

  return count;
}
