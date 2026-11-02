#ifndef TYPES_HPP
#define TYPES_HPP

#include <istream>
#include <ostream>
#include <ios>
#include <string>

namespace hvostov {

  class IOguard {
  public:
    explicit IOguard(std::basic_ios< char >& s);
    ~IOguard();
    IOguard(const IOguard&) = delete;
    IOguard& operator=(const IOguard&) = delete;

  private:
    std::basic_ios< char >& s_;
    std::streamsize width_;
    std::streamsize precision_;
    std::basic_ios< char >::fmtflags fmt_;
    char fill_;
  };

  struct shape_t {
    int rx = 0;
    int ry = 0;
    int x = 0;
    int y = 0;
    double angle = 0.0;
    bool is_ellipse = false;
  };

  struct frame_t {
    double min_x = 0.0;
    double max_x = 0.0;
    double min_y = 0.0;
    double max_y = 0.0;
  };

  struct calc_state_t {
    std::string process_name;
    bool done = false;
    bool canceled = false;
    double result = 0.0;
    double time_ms = 0.0;
  };

  std::istream& operator>>(std::istream& in, shape_t& dest);
  std::ostream& operator<<(std::ostream& out, const shape_t& rhs);
  std::ostream& operator<<(std::ostream& out, const frame_t& rhs);
  std::ostream& operator<<(std::ostream& out, const calc_state_t& rhs);

}

#endif
