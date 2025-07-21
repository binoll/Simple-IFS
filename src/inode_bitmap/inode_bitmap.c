#include "inode_bitmap.h"
#include <string.h>

// Рассчитываем смещение в битовой карте
static inline void get_bitmap_offset(struct superblock* sb, uint32_t inode_idx,
                                     uint32_t* byte_offset,
                                     uint8_t* bit_offset) {
  *byte_offset = inode_idx / 8;
  *bit_offset = inode_idx % 8;
}

void inode_bitmap_init(struct superblock* sb, uint8_t* bitmap) {
  // Обнуляем всю битовую карту
  uint32_t bm_size = bitmap_blocks(sb->inode_count, sb->block_size) * sb->
                     block_size;
  memset(bitmap, 0, bm_size);

  // Помечаем inode 0 как занятый (не используется)
  uint32_t byte_offset;
  uint8_t bit_offset;
  get_bitmap_offset(sb, 0, &byte_offset, &bit_offset);
  bitmap[byte_offset] |= (1 << bit_offset);

  // Помечаем корневой inode как занятый
  get_bitmap_offset(sb, sb->root_inode, &byte_offset, &bit_offset);
  bitmap[byte_offset] |= (1 << bit_offset);

  // Обновляем счетчик в суперблоке
  sb->free_inodes = sb->inode_count - 2;
}

bool inode_allocated(struct superblock* sb, uint8_t* bitmap,
                     uint32_t inode_idx) {
  if (inode_idx >= sb->inode_count) return true;

  uint32_t byte_offset;
  uint8_t bit_offset;
  get_bitmap_offset(sb, inode_idx, &byte_offset, &bit_offset);

  return bitmap[byte_offset] >> bit_offset & 1;
}

int allocate_inode(struct superblock* sb, uint8_t* bitmap) {
  // Находим первый свободный inode
  for (uint32_t i = 1; i < sb->inode_count; i++) {
    if (!inode_allocated(sb, bitmap, i)) {
      uint32_t byte_offset;
      uint8_t bit_offset;
      get_bitmap_offset(sb, i, &byte_offset, &bit_offset);

      // Помечаем как занятый
      bitmap[byte_offset] |= 1 << bit_offset;
      sb->free_inodes--;
      return i;
    }
  }
  return -1; // Нет свободных inode
}

void free_inode(struct superblock* sb, uint8_t* bitmap, uint32_t inode_idx) {
  if (inode_idx < 1 || inode_idx >= sb->inode_count) return;

  uint32_t byte_offset;
  uint8_t bit_offset;
  get_bitmap_offset(sb, inode_idx, &byte_offset, &bit_offset);

  // Сбрасываем бит
  bitmap[byte_offset] &= ~(1 << bit_offset);
  sb->free_inodes++;
}

uint32_t count_free_inodes(struct superblock* sb, uint8_t* bitmap) {
  uint32_t count = 0;
  const uint32_t total_bytes = (sb->inode_count + 7) / 8;

  // Перебираем все биты (кроме inode 0)
  for (uint32_t i = 1; i < sb->inode_count; i++) {
    if (!inode_allocated(sb, bitmap, i)) {
      count++;
    }
  }

  return count;
}