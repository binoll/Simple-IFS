#include "inode_table.h"
#include <string.h>
#include "../debug/debug.h"

void init_inode_table(struct superblock* sb, void* table) {
    sifs_debug("Инициализация таблицы inode\n");

    // Рассчет общего размера таблицы
    uint32_t table_size = sb->count_inode_table_blocks * sb->block_size;

    // Обнуление всей таблицы
    memset(table, 0, table_size);
    sifs_debug("Таблица inode обнулена (%u байт)\n", table_size);

    // Инициализация корневого inode (индекс 1)
    struct inode root;
    init_inode(&root, S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR, 0, 0);
    root.links = 2; // '.' и '..'

    // Сохранение корневого inode в таблице
    if (!write_inode(sb, table, sb->root_inode, &root)) {
        sifs_debug("Ошибка инициализации корневого inode\n");
        return;
    }

    sifs_debug("Корневой inode инициализирован\n");
}

bool read_inode(struct superblock* sb,
                void* table,
                uint32_t inode_idx,
                struct inode* node) {
    // Проверка валидности индекса
    if (inode_idx == 0 || inode_idx >= sb->count_inodes) {
        sifs_debug("Недопустимый индекс inode для чтения: %u\n", inode_idx);
        return false;
    }

    // Рассчет позиции inode
    uint32_t block_offset, byte_offset;
    get_inode_position(sb, inode_idx, &block_offset, &byte_offset);

    // Указатель на блок с inode
    uint8_t* block_ptr = (uint8_t*)table + block_offset * sb->block_size;

    // Копирование данных inode
    memcpy(node, block_ptr + byte_offset, sizeof(struct inode));

    // Проверка магического числа
    if (node->magic != INODE_MAGIC) {
        sifs_debug("Inode %u поврежден (неверное магическое число: 0x%X)\n",
                  inode_idx, node->magic);
        return false;
    }

    sifs_debug("Inode %u прочитан успешно\n", inode_idx);
    return true;
}

bool write_inode(struct superblock* sb,
                 void* table,
                 uint32_t inode_idx,
                 const struct inode* node) {
    // Проверка валидности индекса
    if (inode_idx == 0 || inode_idx >= sb->count_inodes) {
        sifs_debug("Недопустимый индекс inode для записи: %u\n", inode_idx);
        return false;
    }

    // Обновление времени изменения
    time_t current_time = time(NULL);
    ((struct inode*)node)->ctime = current_time;

    // Рассчет позиции inode
    uint32_t block_offset, byte_offset;
    get_inode_position(sb, inode_idx, &block_offset, &byte_offset);

    // Указатель на блок с inode
    uint8_t* block_ptr = (uint8_t*)table + block_offset * sb->block_size;

    // Копирование данных inode
    memcpy(block_ptr + byte_offset, node, sizeof(struct inode));

    sifs_debug("Inode %u записан успешно (изменен: %ld)\n",
              inode_idx, current_time);
    return true;
}

static inline void get_inode_position(struct superblock* sb,
                                      uint32_t inode_idx,
                                      uint32_t* block_offset,
                                      uint32_t* byte_offset) {
    // Общее смещение в байтах
    uint32_t total_offset = inode_idx * sb->inode_size;

    // Смещение блока относительно начала таблицы
    *block_offset = total_offset / sb->block_size;

    // Смещение внутри блока
    *byte_offset = total_offset % sb->block_size;

    // Проверка переполнения
    if (*block_offset >= sb->count_inode_table_blocks) {
        sifs_debug("Ошибка: inode %u выходит за пределы таблицы\n", inode_idx);
        *block_offset = 0;
        *byte_offset = 0;
    }

    sifs_debug("Позиция inode %u: блок=%u, смещение=%u\n",
              inode_idx, *block_offset, *byte_offset);
}