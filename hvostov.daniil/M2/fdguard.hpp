#ifndef FDGUARD_HPP
#define FDGUARD_HPP

#include <unistd.h>

class FdGuard {
public:
  explicit FdGuard(int f):
    fd_(f)
  {}

  FdGuard(const FdGuard&) = delete;
  FdGuard& operator=(const FdGuard&) = delete;

  ~FdGuard()
  {
    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

  explicit operator bool() const noexcept
  {
    return fd_ >= 0;
  }

  int get() const noexcept
  {
    return fd_;
  }

  void close()
  {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

  int release() noexcept
  {
    const int t = fd_;
    fd_ = -1;
    return t;
  }

private:
  int fd_;
};

#endif
