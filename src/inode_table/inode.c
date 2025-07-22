#include "inode.h"
#include "../debug/debug.h"
#include <string.h>
#include <time.h>
#include <stdbool.h>

void init_inode(struct inode* node, uint32_t mode, uint32_t uid, uint32_t gid) {
    memset(node, 0, sizeof(struct inode));

    // Установка обязательных полей
    node->magic = INODE_MAGIC;     // Магическое число для идентификации
    node->mode = mode;             // Тип файла + права доступа
    // Для директорий начальное количество ссылок = 2 ('.' и '..')
    // Для остальных типов файлов - 1
    node->links = mode & S_IFDIR ? 2 : 1;
    node->uid = uid;               // Идентификатор владельца
    node->gid = gid;               // Идентификатор группы

    // Установка временных меток
    time_t now = time(NULL);       // Текущее системное время
    node->atime = now;             // Время последнего доступа
    node->mtime = now;             // Время последнего изменения
    node->ctime = now;             // Время создания/изменения статуса

    sifs_debug("Инициализирован %s: права=%03o, uid=%u, gid=%u, ссылок=%u\n",
               inode_type_str(node), mode & 0777, uid, gid, node->links);
}

bool inode_valid(const struct inode* node) {
    if (node->magic != INODE_MAGIC) {
        sifs_debug("Некорректное магическое число inode: 0x%08X (ожидалось 0x%08X)\n",
                   node->magic, INODE_MAGIC);
        return false;
    }

    // Проверка типа файла (должен быть REG, DIR или LNK)
    uint32_t file_type = node->mode & 0x0F;
    if (file_type != S_IFREG && file_type != S_IFDIR && file_type != S_IFLNK) {
        sifs_debug("Недопустимый тип файла: 0x%X\n", file_type);
        return false;
    }

    // Проверка количества ссылок (не может быть нулевым)
    if (node->links == 0) {
        sifs_debug("Некорректное количество ссылок: 0\n");
        return false;
    }

    return true;
}

const char* inode_type_str(const struct inode* node) {
    switch (node->mode & 0x0F) {
        case S_IFREG: return "файл";        // Обычный файл
        case S_IFDIR: return "директория";  // Каталог
        case S_IFLNK: return "ссылка";      // Символическая ссылка
        default: return "неизвестный";      // Неподдерживаемый тип
    }
}

uint32_t inode_block_count(const struct inode* node, uint32_t block_size) {
    // Расчет количества блоков данных (с округлением вверх)
    uint32_t data_blocks = (node->size + block_size - 1) / block_size;
    uint32_t total_blocks = data_blocks;

    // Если файл использует косвенную адресацию, добавляем 1 блок для указателей
    if (data_blocks > DIRECT_BLOCKS) {
        total_blocks++;
    }

    sifs_debug("Расчет блоков: %u блоков данных + %u косвенный = %u всего\n",
               data_blocks, (data_blocks > DIRECT_BLOCKS) ? 1 : 0, total_blocks);

    return total_blocks;
}

uint32_t inode_get_block(const struct inode* node, uint32_t block_index) {
    // Обработка прямых блоков (индексы 0-11)
    if (block_index < DIRECT_BLOCKS) {
        return node->direct[block_index];
    }

    // Проверка наличия косвенного блока
    if (node->indirect == 0) {
        sifs_debug("Для блока %u не выделен косвенный блок\n", block_index);
        return 0;  // Косвенный блок не выделен
    }

    // Возвращаем номер косвенного блока (фактический поиск указателя будет в вызывающем коде)
    return node->indirect;
}

void inode_update_atime(struct inode* node) {
    node->atime = time(NULL);  // Установка текущего времени
    sifs_debug("Обновлено время доступа для inode\n");
}

void inode_update_mtime(struct inode* node) {
    node->mtime = time(NULL);  // Установка текущего времени
    sifs_debug("Обновлено время изменения для inode\n");
}

bool inode_check_permission(const struct inode* node,
                            uint32_t uid, uint32_t gid, uint32_t mode) {
    // Суперпользователь всегда имеет полный доступ
    if (uid == 0) {
        sifs_debug("Доступ root предоставлен\n");
        return true;
    }

    // Определяем категорию пользователя
    uint32_t eff_perms;
    if (node->uid == uid) {
        // Владелец: извлекаем биты 8-6 (S_IRWXU)
        eff_perms = (node->mode & S_IRWXU) >> 6;
    } else if (node->gid == gid) {
        // Член группы: извлекаем биты 5-3 (S_IRWXG)
        eff_perms = (node->mode & S_IRWXG) >> 3;
    } else {
        // Остальные: извлекаем биты 2-0 (S_IRWXO)
        eff_perms = node->mode & S_IRWXO;
    }

    // Проверяем ВСЕ запрошенные права
    bool access_granted = (eff_perms & mode) == mode;

    if (access_granted) {
        sifs_debug("Доступ разрешен: req %03o, eff %03o, file %03o\n",
                  mode, eff_perms, node->mode & (S_IRWXU|S_IRWXG|S_IRWXO));
    } else {
        sifs_debug("Доступ разрешен: uid=%u (file uid=%u), gid=%u (file gid=%u)\n"
                  "  requested: %03o, effective: %03o, file mode: %03o\n",
                  uid, node->uid, gid, node->gid,
                  mode, eff_perms, node->mode & (S_IRWXU|S_IRWXG|S_IRWXO));
    }

    return access_granted;
}
