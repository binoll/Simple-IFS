#include "mkfs.h"
#include "../superblock/superblock.h"
#include "../inode_bitmap/inode_bitmap.h"
#include "../blocks_bitmap/blocks_bitmap.h"
#include "../inode_table/inode.h"
#include "../image/image.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

int32_t mkfs(const char* filename, uint32_t size) {
    // Создаем файл-образ
    if (image_open(filename, 1) < 0) return -1;

    // Инициализируем суперблок
    struct superblock sb;
    init_superblock(&sb, size);

    // Записываем суперблок
    image_write(&sb, sizeof(sb), 0);

    // Вычисляем размеры областей
    size_t inode_bitmap_size = sb.count_inode_bitmap_blocks * sb.block_size;
    size_t block_bitmap_size = sb.count_block_bitmap_blocks * sb.block_size;
    size_t inode_table_size = sb.count_inode_table_blocks * sb.block_size;

    // Выделяем память под метаданные
    uint8_t* inode_bitmap = malloc(inode_bitmap_size);
    uint8_t* block_bitmap = malloc(block_bitmap_size);
    uint8_t* inode_table = malloc(inode_table_size);

    // Инициализируем битовые карты
    memset(inode_bitmap, 0, inode_bitmap_size);
    memset(block_bitmap, 0, block_bitmap_size);
    inode_bitmap_init(&sb, inode_bitmap);
    block_bitmap_init(&sb, block_bitmap);

    // Инициализируем таблицу inode
    memset(inode_table, 0, inode_table_size);
    struct inode* root_inode = (struct inode*)(inode_table + sb.root_inode * sb.inode_size);
    root_inode->mode = S_IFDIR;
    root_inode->links = 2;
    root_inode->size = 0;
    time_t now = time(NULL);
    root_inode->atime = now;
    root_inode->mtime = now;
    root_inode->ctime = now;

    // Записываем метаданные в образ
    off_t offset = sb.block_size; // После суперблока

    image_write(inode_bitmap, inode_bitmap_size, offset);
    offset += (off_t)inode_bitmap_size;

    image_write(block_bitmap, block_bitmap_size, offset);
    offset += (off_t)block_bitmap_size;

    image_write(inode_table, inode_table_size, offset);

    // Освобождаем ресурсы
    free(inode_bitmap);
    free(block_bitmap);
    free(inode_table);
    image_close();

    return 0;
}