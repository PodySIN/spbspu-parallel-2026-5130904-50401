#ifndef IPC_HPP
#define IPC_HPP

#include <cstddef>
#include "types.hpp"

namespace hvostov {

  size_t send(int& err, int wr, const char* b, size_t k);
  size_t recv(int& err, int rd, char* b, size_t k);

  constexpr size_t CALC_NAME_SIZE = 64;
  constexpr size_t MAX_SHAPES = 4096;
  constexpr size_t MAX_SLOTS = 16;

  struct AreaRequestHeader {
    char calc_name[CALC_NAME_SIZE];
    int threads;
    size_t tries;
    size_t shape_count;
    size_t seed;
  };

  struct AreaResultHeader {
    char calc_name[CALC_NAME_SIZE];
    double result;
    double time_ms;
  };

  struct ShmSlot {
    AreaRequestHeader req;
    AreaResultHeader res;
    shape_t shapes[MAX_SHAPES];
  };

  struct ShmLayout {
    ShmSlot slots[MAX_SLOTS];
    size_t write_idx;
    size_t read_idx;
  };

}

#endif
