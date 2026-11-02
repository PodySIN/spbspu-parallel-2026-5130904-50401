#include "calculation.hpp"
#include <algorithm>
#include <limits>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
#include <vector>

hvostov::frame_t hvostov::calculateFrame(const shape_t& s)
{
  frame_t frame{};
  if (!s.is_ellipse) {
    frame.min_x = s.x - s.rx;
    frame.max_x = s.x + s.rx;
    frame.min_y = s.y - s.rx;
    frame.max_y = s.y + s.rx;
  } else {
    const double c = std::cos(s.angle);
    const double sn = std::sin(s.angle);
    const double a = s.rx;
    const double b = s.ry;
    const double half_w = std::sqrt(a * a * c * c + b * b * sn * sn);
    const double half_h = std::sqrt(a * a * sn * sn + b * b * c * c);
    frame.min_x = s.x - half_w;
    frame.max_x = s.x + half_w;
    frame.min_y = s.y - half_h;
    frame.max_y = s.y + half_h;
  }
  return frame;
}

hvostov::frame_t hvostov::calculateFrame(const std::vector< shape_t >& shapes)
{
  const double inf = std::numeric_limits< double >::infinity();
  frame_t frame{inf, -inf, inf, -inf};
  for (const shape_t& s : shapes) {
    const frame_t f = calculateFrame(s);
    frame.min_x = std::min(frame.min_x, f.min_x);
    frame.max_x = std::max(frame.max_x, f.max_x);
    frame.min_y = std::min(frame.min_y, f.min_y);
    frame.max_y = std::max(frame.max_y, f.max_y);
  }
  return frame;
}

bool hvostov::isInside(double px, double py, const shape_t& s)
{
  const double dx = px - s.x;
  const double dy = py - s.y;

  if (!s.is_ellipse) {
    return dx * dx + dy * dy <= static_cast< double >(s.rx) * s.rx;
  }

  const double c = std::cos(-s.angle);
  const double sn = std::sin(-s.angle);
  const double lx = dx * c - dy * sn;
  const double ly = dx * sn + dy * c;

  const double nx = lx / s.rx;
  const double ny = ly / s.ry;
  return nx * nx + ny * ny <= 1.0;
}

double hvostov::calcArea(
    const std::vector< shape_t >& shapes, size_t threads, size_t tries, size_t seed, double& time_ms)
{
  if (shapes.empty() || tries == 0) {
    time_ms = 0.0;
    return 0.0;
  }

  const frame_t frame = calculateFrame(shapes);
  const double width = frame.max_x - frame.min_x;
  const double height = frame.max_y - frame.min_y;
  const double rect_area = width * height;

  if (rect_area <= 0.0) {
    time_ms = 0.0;
    return 0.0;
  }

  const size_t nthreads = threads > 0 ? threads : 1;
  const size_t tries_per = tries / nthreads;
  const size_t remainder = tries % nthreads;

  std::vector< size_t > local_hits(nthreads, 0);
  std::vector< std::thread > workers;
  workers.reserve(nthreads);

  const auto start = std::chrono::high_resolution_clock::now();

  for (size_t t = 0; t < nthreads; ++t) {
    const size_t local_tries = tries_per + (t < remainder ? 1 : 0);
    const size_t thread_seed = seed + t;
    size_t& hits_ref = local_hits[t];

    workers.emplace_back(
        [&, local_tries, thread_seed]()
        {
          std::default_random_engine eng(static_cast< unsigned >(thread_seed));
          const double eng_max = static_cast< double >(eng.max());
          const double kx = frame.max_x - frame.min_x;
          const double ky = frame.max_y - frame.min_y;

          size_t count = 0;
          for (size_t i = 0; i < local_tries; ++i) {
            const double px = frame.min_x + (static_cast< double >(eng()) / eng_max) * kx;
            const double py = frame.min_y + (static_cast< double >(eng()) / eng_max) * ky;
            for (const shape_t& s : shapes) {
              if (isInside(px, py, s)) {
                ++count;
                break;
              }
            }
          }
          hits_ref = count;
        });
  }

  for (size_t i = 0; i < workers.size(); ++i) {
    workers[i].join();
  }

  const auto end = std::chrono::high_resolution_clock::now();
  time_ms = std::chrono::duration< double, std::milli >(end - start).count();

  size_t total_hits = 0;
  for (size_t i = 0; i < local_hits.size(); ++i) {
    total_hits += local_hits[i];
  }

  const double ratio = static_cast< double >(total_hits) / static_cast< double >(tries);
  return ratio * rect_area;
}
