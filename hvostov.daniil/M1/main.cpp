#include <algorithm>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace hvostov
{
  struct shape_t
  {
    double hor_radius, vert_radius, centerx, centery;
  };

  struct rect_t
  {
    double min_x, max_x, min_y, max_y;
  };

  bool isInside(double px, double py, const shape_t& s);
  size_t countInsides(double px, double py, const std::vector< shape_t >& shapes);
  rect_t getRect(const std::vector< shape_t >& shapes);
  std::pair< size_t, size_t > calc(const std::vector< shape_t >& shapes, rect_t rect, size_t tries, size_t seed);
  std::pair< double, double > area(const std::vector< shape_t >& shapes, size_t threads, size_t tries, size_t seed);
  std::vector< shape_t > inputShapes(std::istream& in);
}

int main(int argc, char** argv)
{
  if (argc != 3 && argc != 4)
  {
    std::cerr << "Invalid amount of arguments\n";
    return 1;
  }

  long long threads = 0, tries = 0, seed = 0;
  try
  {
    threads = std::stoll(argv[1]);
    tries = std::stoll(argv[2]);
    if (argc == 4)
    {
      seed = std::stoll(argv[3]);
    }
  }
  catch (...)
  {
    std::cerr << "Wrong input\n";
    return 1;
  }
  if (tries <= 0)
  {
    std::cerr << "tries must be positive\n";
    return 1;
  }
  if (threads < 0 || seed < 0)
  {
    std::cerr << "threads and seed must not be negative\n";
    return 1;
  }
  if (threads == 0)
  {
    threads = 1;
  }

  std::vector< hvostov::shape_t > shapes;
  try
  {
    shapes = hvostov::inputShapes(std::cin);
  }
  catch (const std::runtime_error& e)
  {
    std::cerr << e.what() << "\n";
    return 1;
  }

  if (shapes.empty())
  {
    std::cout << 0.0 << " " << 0.0 << "\n";
    return 0;
  }

  const std::pair< double, double > result = hvostov::area(shapes, threads, tries, seed);
  std::cout << result.first << " " << result.second << "\n";
}

bool hvostov::isInside(double px, double py, const shape_t& s)
{
  const double dx = (px - s.centerx) / s.hor_radius;
  const double dy = (py - s.centery) / s.vert_radius;
  return (dx * dx + dy * dy) <= 1.0;
}

size_t hvostov::countInsides(double px, double py, const std::vector< shape_t >& shapes)
{
  return std::count_if(shapes.begin(), shapes.end(),
      [&](const shape_t& s)
      {
        return isInside(px, py, s);
      });
}

hvostov::rect_t hvostov::getRect(const std::vector< shape_t >& shapes)
{
  const double inf = std::numeric_limits< double >::infinity();
  rect_t rect = {inf, -inf, inf, -inf};
  for (const shape_t& s : shapes)
  {
    rect.min_x = std::min(rect.min_x, s.centerx - s.hor_radius);
    rect.max_x = std::max(rect.max_x, s.centerx + s.hor_radius);
    rect.min_y = std::min(rect.min_y, s.centery - s.vert_radius);
    rect.max_y = std::max(rect.max_y, s.centery + s.vert_radius);
  }
  return rect;
}

std::pair< size_t, size_t > hvostov::calc(const std::vector< shape_t >& shapes, rect_t rect, size_t tries, size_t seed)
{
  std::default_random_engine gen(seed);
  std::uniform_real_distribution< double > dx(rect.min_x, rect.max_x);
  std::uniform_real_distribution< double > dy(rect.min_y, rect.max_y);

  size_t count_in_any = 0;
  size_t count_in_all = 0;
  for (size_t i = 0; i < tries; i++)
  {
    const double px = dx(gen);
    const double py = dy(gen);
    const size_t n = countInsides(px, py, shapes);
    if (n == shapes.size())
    {
      ++count_in_all;
    }
    if (n != 0)
    {
      ++count_in_any;
    }
  }
  return {count_in_any, count_in_all};
}

std::pair< double, double > hvostov::area(
    const std::vector< shape_t >& shapes, size_t threads, size_t tries, size_t seed)
{
  if (shapes.empty() || tries == 0)
  {
    return {0.0, 0.0};
  }
  if (threads == 0)
  {
    threads = 1;
  }
  if (threads > tries)
  {
    threads = tries;
  }

  const rect_t rect = getRect(shapes);
  const double rectArea = (rect.max_x - rect.min_x) * (rect.max_y - rect.min_y);

  const size_t batch = tries / threads;
  const size_t remainder = tries % threads;

  std::vector< std::future< std::pair< size_t, size_t > > > futures;
  futures.reserve(threads);
  for (size_t i = 0; i < threads; i++)
  {
    const size_t n = batch + (i < remainder ? 1 : 0);
    futures.push_back(std::async(std::launch::async, calc, std::cref(shapes), rect, n, seed + i));
  }

  size_t count_in_any = 0;
  size_t count_in_all = 0;
  for (auto& f : futures)
  {
    const std::pair< size_t, size_t > r = f.get();
    count_in_any += r.first;
    count_in_all += r.second;
  }

  const double union_area = rectArea * static_cast< double >(count_in_any) / static_cast< double >(tries);
  const double inter_area = rectArea * static_cast< double >(count_in_all) / static_cast< double >(tries);
  return {union_area, inter_area};
}

std::vector< hvostov::shape_t > hvostov::inputShapes(std::istream& in)
{
  std::vector< shape_t > shapes;
  double hor_radius = 0, vert_radius = 0, centerx = 0, centery = 0;

  while (in >> hor_radius)
  {
    if (!(in >> vert_radius >> centerx >> centery))
    {
      throw std::runtime_error("Cannot parse shape");
    }
    if (hor_radius <= 0 || vert_radius < 0)
    {
      throw std::runtime_error("Invalid shape parameters");
    }
    const double rx = hor_radius;
    const double ry = (vert_radius == 0) ? rx : vert_radius;
    shapes.push_back({rx, ry, centerx, centery});
  }

  if (!in.eof())
  {
    throw std::runtime_error("Cannot parse shape");
  }
  return shapes;
}
