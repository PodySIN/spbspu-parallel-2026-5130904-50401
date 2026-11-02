#ifndef SHM_HPP
#define SHM_HPP

#include <cstddef>
#include <string>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

class ShmGuard {
public:
  ShmGuard(const std::string& name, size_t size, bool create);
  ~ShmGuard();

  ShmGuard(const ShmGuard&) = delete;
  ShmGuard& operator=(const ShmGuard&) = delete;

  void* get() const noexcept
  {
    return addr_;
  }

  size_t size() const noexcept
  {
    return size_;
  }

  const std::string& name() const noexcept
  {
    return name_;
  }

private:
  std::string name_;
  size_t size_;
  void* addr_;
  bool owner_;
};

#endif
