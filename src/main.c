#include "mkfs/mkfs.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Запустите: %s <imagefile> <size>\n", argv[0]);
    return 1;
  }

  const char* filename = argv[1];
  uint32_t size = atoi(argv[2]);

  if (mkfs(filename, size)) {
    fprintf(stderr, "Не удалось создать файловую систему\n");
    return 1;
  }

  printf("Файловая система успешно создана: %s\n", filename);
  return 0;
}
