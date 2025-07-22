#include "superblock.h"
#include <string.h>
#include "../debug/debug.h"

static inline uint8_t superblock_valid(const struct superblock* sb) {
  uint8_t valid = sb->magic == FS_MAGIC;
  sifs_debug("Проверка суперблока: %s\n", valid ? "валиден" : "невалиден");
  return valid;
}

static inline uint32_t get_bitmap_blocks(const uint32_t bits,
                                         const uint32_t block_size) {
  uint32_t blocks = (bits + block_size * 8 - 1) / (block_size * 8);
  sifs_debug("Расчет блоков битмапа: биты=%u, размер_блока=%u, результат=%u\n",
      bits, block_size, blocks);
  return blocks;
}

static inline void init_superblock(struct superblock* sb,
                                   const uint32_t space_size) {
    const uint32_t block_size = DEFAULT_BLOCK_SIZE;
    const uint32_t inode_size = DEFAULT_INODE_SIZE;

    // Расчет общего количества блоков в разделе
    uint32_t total_blocks = (space_size + block_size - 1) / block_size;

    // Минимальные требования к размеру ФС
    if (total_blocks < 10) {
        sifs_debug("Ошибка: ФС слишком мала. Требуется минимум 10 блоков.\n");
        return;
    }

    // 1 inode на 4 блока данных
    uint32_t inode_count = total_blocks / 4;
    if (inode_count < 2) inode_count = 2;  // Минимум 2 inode

    // Расчет метаданных с итеративным подбором
    uint32_t inode_bitmap_blocks, block_bitmap_blocks, inode_table_blocks;
    uint32_t total_meta_blocks;

    do {
        // Расчет блоков для структур метаданных
        inode_bitmap_blocks = get_bitmap_blocks(inode_count, block_size);
        block_bitmap_blocks = get_bitmap_blocks(total_blocks, block_size);
        inode_table_blocks = (inode_count * inode_size + block_size - 1) / block_size;

        // Суммарный размер метаданных
        total_meta_blocks = 1 +  // superblock
            inode_bitmap_blocks +
            block_bitmap_blocks +
            inode_table_blocks;

        // Корректировка количества inode при переполнении
        if (total_meta_blocks >= total_blocks && inode_count > 2) {
            inode_count--;
        } else {
            break;
        }
    } while (1);

    // Проверка вместимости метаданных
    if (total_meta_blocks >= total_blocks) {
        sifs_debug("Ошибка: Недостаточно места под метаданные\n");
        return;
    }

    // Заполнение структуры суперблока
    memset(sb, 0, sizeof(struct superblock));
    sb->magic = FS_MAGIC;
    strncpy(sb->fs_name, FS_NAME, MAX_FS_NAME);
    sb->block_size = block_size;
    sb->inode_size = inode_size;

    // Учет ресурсов
    sb->count_inodes = inode_count;
    sb->count_free_inodes = inode_count - 1;  // inode 0 зарезервирован
    sb->count_blocks = total_blocks;
    sb->count_free_blocks = total_blocks - total_meta_blocks;

    // Разметка областей
    sb->count_inode_bitmap_blocks = inode_bitmap_blocks;
    sb->count_block_bitmap_blocks = block_bitmap_blocks;
    sb->count_inode_table_blocks = inode_table_blocks;
    sb->count_blocks_data = total_blocks - total_meta_blocks;

    // Физическая карта расположения
    sb->first_inode_bitmap_block = 1;  // Блок сразу после суперблока
    sb->first_block_bitmap_block = sb->first_inode_bitmap_block + inode_bitmap_blocks;
    sb->first_inode_table_block = sb->first_block_bitmap_block + block_bitmap_blocks;
    sb->first_block_data = sb->first_inode_table_block + inode_table_blocks;

    // Системные параметры
    sb->root_inode = 1;         // Корневой каталог в inode 1
    sb->clean_shutdown = 1;     // Флаг "чистого" выключения

    // Отладочный вывод
    sifs_debug("Суперблок инициализирован успешно\n");
    sifs_debug("Всего блоков: %u\n", total_blocks);
    sifs_debug("Метаблоков: %u\n", total_meta_blocks);
    sifs_debug("Inode: %u (%u свободно)\n", inode_count, inode_count - 1);
    sifs_debug("Блоков данных: %u\n", total_blocks - total_meta_blocks);
    sifs_debug("Расположение: [0] Суперблок, [%u] Битмап inode (%u блоков), "
              "[%u] Битмап блоков (%u блоков), [%u] Таблица inode (%u блоков), "
              "[%u] Данные\n",
              sb->first_inode_bitmap_block, inode_bitmap_blocks,
              sb->first_block_bitmap_block, block_bitmap_blocks,
              sb->first_inode_table_block, inode_table_blocks,
              sb->first_block_data);
}
