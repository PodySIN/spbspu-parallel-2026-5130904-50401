#include "ipc.hpp"
#include <unistd.h>
#include <cerrno>

size_t hvostov::send(int& err, int wr, const char* b, size_t k)
{
  size_t r = 0;
  while (r < k) {
    err = static_cast< int >(::write(wr, b + r, k - r));
    if (err < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    if (err == 0) {
      break;
    }
    r += static_cast< size_t >(err);
  }
  return r;
}

size_t hvostov::recv(int& err, int rd, char* b, size_t k)
{
  size_t r = 0;
  while (r < k) {
    err = static_cast< int >(::read(rd, b + r, k - r));
    if (err < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    if (err == 0) {
      break;
    }
    r += static_cast< size_t >(err);
  }
  return r;
}
