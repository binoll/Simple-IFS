#include "super.h"
#include <string.h>
#include "../debug/debug.h"

static inline uint8_t sb_valid(const struct superblock* sb) {
  uint8_t valid = sb->magic == FS_MAGIC;
  sifs_debug("Superblock validation: %s\n", valid ? "valid" : "invalid");
  return valid;
}

static inline uint32_t bitmap_blocks(const uint32_t bits,
                                     const uint32_t block_size) {
  uint32_t blocks = (bits + block_size * 8 - 1) / (block_size * 8);
  sifs_debug("Bitmap blocks calculation: bits=%u, block_size=%u, result=%u\n",
             bits, block_size, blocks);
  return blocks;
}

static inline void init_superblock(struct superblock* sb,
                                   const uint32_t total_blocks,
                                   const uint32_t inode_count) {
  sifs_debug("Initializing superblock: total_blocks=%u, inode_count=%u\n",
             total_blocks, inode_count);

  // Проверка входных параметров
  sifs_assert(total_blocks > 10 && "Filesystem too small (min 10 blocks)");
  sifs_assert(inode_count > 1 && "Not enough inodes (min 2 required)");

  // Обнуление и базовая инициализация
  memset(sb, 0, sizeof(*sb));
  sb->magic = FS_MAGIC;
  strncpy(sb->fs_name, FS_NAME, MAX_FS_NAME);
  sb->block_size = DEFAULT_BLOCK_SIZE;
  sb->inode_size = DEFAULT_INODE_SIZE;
  sb->inode_count = inode_count;
  sb->free_inodes = inode_count > 0 ? inode_count - 1 : 0;
  sb->block_count = total_blocks;
  sb->root_inode = 1; // Корневой каталог - inode 1
  sb->clean_shutdown = 1;

  sifs_debug(
      "Basic fields set: magic=0x%X, fs_name='%s', block_size=%u, inode_size=%u\n",
      sb->magic, sb->fs_name, sb->block_size, sb->inode_size);

  // 1. Расчет блоков для битовых карт
  sb->inode_bitmap_block = 1; // Блок сразу после суперблока
  const uint32_t inode_bitmap_blocks = bitmap_blocks(
      inode_count, DEFAULT_BLOCK_SIZE);

  sifs_debug("Inode bitmap: starts at block %u, blocks=%u\n",
             sb->inode_bitmap_block, inode_bitmap_blocks);

  sb->block_bitmap_block = sb->inode_bitmap_block + inode_bitmap_blocks;
  const uint32_t block_bitmap_blocks = bitmap_blocks(
      total_blocks, DEFAULT_BLOCK_SIZE);

  sifs_debug("Block bitmap: starts at block %u, blocks=%u\n",
             sb->block_bitmap_block, block_bitmap_blocks);

  // 2. Расчет блоков для таблицы inode (с округлением вверх)
  const uint32_t inode_table_blocks =
      ((uint64_t)inode_count * DEFAULT_INODE_SIZE + DEFAULT_BLOCK_SIZE - 1)
      / DEFAULT_BLOCK_SIZE;

  sifs_debug("Inode table: requires %u blocks (%u inodes * %u bytes)\n",
             inode_table_blocks, inode_count, DEFAULT_INODE_SIZE);

  // 3. Расположение таблицы inode
  sb->inode_table_block = sb->block_bitmap_block + block_bitmap_blocks;

  sifs_debug("Inode table starts at block: %u\n", sb->inode_table_block);

  // 4. Первый блок данных
  sb->first_data_block = sb->inode_table_block + inode_table_blocks;

  sifs_debug("First data block: %u\n", sb->first_data_block);

  // 5. Проверка переполнения и расчет свободных блоков
  if (sb->first_data_block > total_blocks) {
    sifs_debug("Metadata overflow! Required %u blocks, available %u\n",
               sb->first_data_block, total_blocks);
    sb->free_blocks = 0;
  } else {
    sb->free_blocks = total_blocks - sb->first_data_block;
    sifs_debug("Free blocks available: %u\n", sb->free_blocks);
  }

  // Проверка целостности суперблока
  sifs_assert(
      sb_valid(sb) && "Superblock validation failed after initialization");

  sifs_debug("Superblock initialization completed successfully\n");
}