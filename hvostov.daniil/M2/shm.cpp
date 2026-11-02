#include "shm.hpp"
#include <cstring>
#include <cerrno>
#include <system_error>

ShmGuard::ShmGuard(const std::string& name, size_t size, bool create):
  name_(name),
  size_(size),
  addr_(MAP_FAILED),
  owner_(create)
{
  const int flags = create ? (O_CREAT | O_RDWR | O_EXCL) : O_RDWR;
  const int fd = shm_open(name.c_str(), flags, 0600);
  if (fd < 0) {
    throw std::system_error(errno, std::generic_category(), "shm_open");
  }

  if (create) {
    if (ftruncate(fd, static_cast< off_t >(size)) != 0) {
      const int e = errno;
      ::close(fd);
      shm_unlink(name.c_str());
      throw std::system_error(e, std::generic_category(), "ftruncate");
    }
  }

  addr_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  ::close(fd);

  if (addr_ == MAP_FAILED) {
    if (owner_) {
      shm_unlink(name.c_str());
    }
    throw std::system_error(errno, std::generic_category(), "mmap");
  }

  if (create) {
    std::memset(addr_, 0, size);
  }
}

ShmGuard::~ShmGuard()
{
  if (addr_ != MAP_FAILED) {
    munmap(addr_, size_);
  }
  if (owner_) {
    shm_unlink(name_.c_str());
  }
}
