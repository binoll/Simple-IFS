#include "mkfs/mkfs.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <imagefile> <size>\n", argv[0]);
    return 1;
  }

  const char* filename = argv[1];
  uint32_t size = atoi(argv[2]);

  if (mkfs(filename, size)) {
    fprintf(stderr, "Failed to create filesystem\n");
    return 1;
  }

  printf("Filesystem created successfully: %s\n", filename);
  return 0;
}
