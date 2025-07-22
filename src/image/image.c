#include "image.h"
#include <fcntl.h>
#include <unistd.h>

static int32_t image_fd = -1;

int32_t image_open(const char* filename, int32_t truncate) {
  int32_t flags = O_RDWR | O_CREAT;
  if (truncate) flags |= O_TRUNC;

  image_fd = open(filename, flags, 0666);
  return image_fd;
}

int32_t image_close(void) {
  if (image_fd != -1) {
    close(image_fd);
    image_fd = -1;
  }
  return 0;
}

ssize_t image_read(void* buffer, size_t size, off_t offset) {
  if (lseek(image_fd, offset, SEEK_SET) == (off_t)-1) return -1;
  return read(image_fd, buffer, size);
}

ssize_t image_write(const void* buffer, size_t size, off_t offset) {
  if (lseek(image_fd, offset, SEEK_SET) == (off_t)-1) return -1;
  return write(image_fd, buffer, size);
}