#include "types.hpp"
#include <iomanip>

hvostov::IOguard::IOguard(std::basic_ios< char >& s):
  s_(s),
  width_(s.width()),
  precision_(s.precision()),
  fmt_(s.flags()),
  fill_(s.fill())
{}

hvostov::IOguard::~IOguard()
{
  s_.width(width_);
  s_.fill(fill_);
  s_.precision(precision_);
  s_.flags(fmt_);
}

std::istream& hvostov::operator>>(std::istream& in, shape_t& dest)
{
  const std::istream::sentry sentry(in);
  if (!sentry) {
    return in;
  }

  int rx = 0;
  int ry = 0;
  int x = 0;
  int y = 0;

  in >> rx >> ry >> x >> y;
  if (in) {
    dest.rx = rx;
    dest.ry = ry;
    dest.x = x;
    dest.y = y;
    dest.angle = 0.0;
    dest.is_ellipse = (ry != 0);
  }
  return in;
}

std::ostream& hvostov::operator<<(std::ostream& out, const shape_t& rhs)
{
  const std::ostream::sentry sentry(out);
  if (!sentry) {
    return out;
  }

  const IOguard guard(out);
  if (!rhs.is_ellipse) {
    out << rhs.rx << " (" << rhs.x << " " << rhs.y << ")";
  } else {
    out << rhs.rx << " " << rhs.ry << " (" << rhs.x << " " << rhs.y << ") " << std::fixed << std::setprecision(3)
        << rhs.angle;
  }
  return out;
}

std::ostream& hvostov::operator<<(std::ostream& out, const frame_t& rhs)
{
  const std::ostream::sentry sentry(out);
  if (!sentry) {
    return out;
  }

  const IOguard guard(out);
  out << "(" << static_cast< long long >(rhs.min_x) << " " << static_cast< long long >(rhs.min_y) << ") ("
      << static_cast< long long >(rhs.max_x) << " " << static_cast< long long >(rhs.max_y) << ")";
  return out;
}

std::ostream& hvostov::operator<<(std::ostream& out, const calc_state_t& rhs)
{
  const std::ostream::sentry sentry(out);
  if (!sentry) {
    return out;
  }

  const IOguard guard(out);
  out << rhs.process_name << " ";
  if (rhs.canceled) {
    out << "<CANCELED>";
  } else if (rhs.done) {
    out << std::fixed << std::setprecision(3) << rhs.time_ms << " " << rhs.result;
  } else {
    out << "<IN PROGRESS>";
  }
  return out;
}
