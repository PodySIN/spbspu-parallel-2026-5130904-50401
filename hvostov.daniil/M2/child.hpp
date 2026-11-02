#ifndef CHILD_HPP
#define CHILD_HPP

#include "ipc.hpp"

namespace hvostov {
  int runChild(size_t seed, int rd, int wr, ShmLayout* layout);
}

#endif
