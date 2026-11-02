#ifndef CALCULATION_HPP
#define CALCULATION_HPP

#include "types.hpp"
#include <vector>
#include <cstddef>

namespace hvostov {
  frame_t calculateFrame(const shape_t& shape);
  frame_t calculateFrame(const std::vector< shape_t >& shapes);

  bool isInside(double px, double py, const shape_t& s);

  double calcArea(const std::vector< shape_t >& shapes, size_t threads, size_t tries, size_t seed, double& time_ms);
}

#endif
