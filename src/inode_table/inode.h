#pragma once

#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#define MAX_FILENAME 255          // Максимальная длина имени файла
#define DIRECT_BLOCKS 12          // Количество прямых указателей на блоки в inode
#define INODE_MAGIC 0x494E4F44    // Магическое число "INOD"

// Типы файлов (битовые флаги)
#define S_IFREG  0x01             // Обычный файл
#define S_IFDIR  0x02             // Каталог
#define S_IFLNK  0x04             // Символическая ссылка

// Маска типа файла (старшие 4 бита)
#define S_IFMT  0xF000  // Маска типа файла

// Типы файлов
#define S_IFREG 0x8000  // Обычный файл
#define S_IFDIR 0x4000  // Каталог
#define S_IFLNK 0xA000  // Символическая ссылка

// Права доступа
#define S_IRUSR 0x0100  // Владелец: чтение
#define S_IWUSR 0x0080  // Владелец: запись
#define S_IXUSR 0x0040  // Владелец: выполнение

#define S_IRGRP 0x0020  // Группа: чтение
#define S_IWGRP 0x0010  // Группа: запись
#define S_IXGRP 0x0008  // Группа: выполнение

#define S_IROTH 0x0004  // Остальные: чтение
#define S_IWOTH 0x0002  // Остальные: запись
#define S_IXOTH 0x0001  // Остальные: выполнение

// Комбинированные права
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)  // 0x01C0
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)  // 0x0038
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)  // 0x0007

// Макросы для проверки типа
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)

// Структура inode
struct inode {
  uint32_t magic;                 // Магическое число (должно совпадать с INODE_MAGIC)
  uint32_t size;                  // Размер файла в байтах
  uint32_t mode;                  // Тип файла (младшие 4 бита) + права доступа (старшие 9 бит)
  uint32_t links;                 // Количество жестких ссылок на inode

  // Временные метки в формате Unix time
  time_t atime;                   // Время последнего доступа (access)
  time_t mtime;                   // Время последней модификации (modify)
  time_t ctime;                   // Время создания/изменения статуса (change)

  // Система адресации блоков данных
  uint32_t direct[DIRECT_BLOCKS]; // Прямые указатели на блоки данных (для первых 12 блоков)
  uint32_t indirect;              // Косвенный блок (хранит указатели на следующие блоки)

  // Владелец файла
  uint32_t uid;                   // Идентификатор пользователя-владельца
  uint32_t gid;                   // Идентификатор группы-владельца
};

// Инициализирует новый inode
extern void init_inode(struct inode* node, uint32_t mode, uint32_t uid, uint32_t gid);

// Проверяет валидность структуры inode (магическое число, тип файла и т.д.)
extern bool inode_valid(const struct inode* node);

// Возвращает строковое представление типа файла
extern const char* inode_type_str(const struct inode* node);

// Рассчитывает количество блоков, занимаемых файлом
extern uint32_t inode_block_count(const struct inode* node, uint32_t block_size);

// Возвращает физический номер блока по логическому индексу
extern uint32_t inode_get_block(const struct inode* node, uint32_t block_index);

// Обновляет время последнего доступа (atime) до текущего времени
extern void inode_update_atime(struct inode* node);

// Обновляет время последней модификации (mtime) до текущего времени
extern void inode_update_mtime(struct inode* node);

// Проверяет права доступа для указанного пользователя/группы
extern bool inode_check_permission(const struct inode* node, uint32_t uid,
                          uint32_t gid, uint32_t mode);
