#include "commands.hpp"
#include <stdexcept>
#include <sys/wait.h>
#include <cerrno>
#include <system_error>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <csignal>
#include <algorithm>
#include "calculation.hpp"
#include "ipc.hpp"
#include "fdguard.hpp"
#include "child.hpp"

namespace {
  long long parseNumber(const std::string& word)
  {
    size_t pos = 0;
    const long long value = std::stoll(word, &pos);
    if (pos != word.size()) {
      throw std::invalid_argument("bad number");
    }
    return value;
  }

  bool validName(const std::string& name)
  {
    if (name.empty()) {
      return false;
    }
    return std::isalpha(static_cast< unsigned char >(name.front())) != 0;
  }

  bool tryReadResult(hvostov::info_t& info, hvostov::process_t& p)
  {
    struct pollfd pfd = {p.from_child, POLLIN, 0};
    if (poll(&pfd, 1, 0) <= 0) {
      return false;
    }

    char sig = 0;
    int err = 0;
    const size_t n = hvostov::recv(err, p.from_child, &sig, 1);
    if (n != 1) {
      return false;
    }

    const hvostov::ShmLayout* const layout = reinterpret_cast< const hvostov::ShmLayout* >(p.shm->get());
    const size_t idx = p.res_idx % hvostov::MAX_SLOTS;
    const hvostov::ShmSlot& slot = layout->slots[idx];

    const std::string calc_name(slot.res.calc_name, strnlen(slot.res.calc_name, sizeof(slot.res.calc_name)));

    auto it = info.calc.find(calc_name);
    if (it != info.calc.end()) {
      it->second.done = true;
      it->second.result = slot.res.result;
      it->second.time_ms = slot.res.time_ms;
    }

    p.res_idx++;
    auto pit = std::find(p.pending.begin(), p.pending.end(), calc_name);
    if (pit != p.pending.end()) {
      p.pending.erase(pit);
    }
    return true;
  }
}

hvostov::Args hvostov::splitLine(const std::string& line)
{
  Args words;
  size_t i = 0;
  while (i < line.size()) {
    while (i < line.size() && std::isspace(static_cast< unsigned char >(line[i]))) {
      ++i;
    }
    if (i >= line.size()) {
      break;
    }
    size_t j = i;
    while (j < line.size() && !std::isspace(static_cast< unsigned char >(line[j]))) {
      ++j;
    }
    words.push_back(line.substr(i, j - i));
    i = j;
  }
  return words;
}

void hvostov::addCircle(std::ostream&, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 5;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  const std::string& name = args[1];
  if (!validName(name)) {
    throw std::logic_error("bad name");
  }
  if (info.db.count(name) || info.set.count(name)) {
    throw std::logic_error("name exists");
  }
  const long long r_ll = parseNumber(args[2]);
  const long long x_ll = parseNumber(args[3]);
  const long long y_ll = parseNumber(args[4]);
  if (r_ll <= 0) {
    throw std::logic_error("bad radius");
  }
  shape_t s{};
  s.rx = static_cast< int >(r_ll);
  s.ry = 0;
  s.x = static_cast< int >(x_ll);
  s.y = static_cast< int >(y_ll);
  s.angle = 0.0;
  s.is_ellipse = false;
  info.db[name] = s;
}

void hvostov::addEllipse(std::ostream&, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 6;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  const std::string& name = args[1];
  if (!validName(name)) {
    throw std::logic_error("bad name");
  }
  if (info.db.count(name) || info.set.count(name)) {
    throw std::logic_error("name exists");
  }
  const long long rx_ll = parseNumber(args[2]);
  const long long ry_ll = parseNumber(args[3]);
  const long long x_ll = parseNumber(args[4]);
  const long long y_ll = parseNumber(args[5]);
  if (rx_ll <= 0 || ry_ll <= 0) {
    throw std::logic_error("bad radii");
  }
  shape_t s{};
  s.rx = static_cast< int >(rx_ll);
  s.ry = static_cast< int >(ry_ll);
  s.x = static_cast< int >(x_ll);
  s.y = static_cast< int >(y_ll);
  s.angle = 0.0;
  s.is_ellipse = true;
  info.db[name] = s;
}

void hvostov::rotate(std::ostream&, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 3;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  const std::string& name = args[1];
  size_t pos = 0;
  const double ksi = std::stod(args[2], &pos);
  if (pos != args[2].size()) {
    throw std::invalid_argument("bad angle");
  }
  auto it = info.db.find(name);
  if (it == info.db.end()) {
    throw std::logic_error("shape not found");
  }
  it->second.angle += ksi;
}

void hvostov::show(std::ostream& out, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 2;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  auto it = info.db.find(args[1]);
  if (it == info.db.end()) {
    throw std::logic_error("shape not found");
  }
  out << it->second << "\n";
}

void hvostov::set(std::ostream&, const Args& args, info_t& info)
{
  constexpr size_t min_args = 3;
  if (args.size() < min_args) {
    throw std::logic_error("bad args count");
  }
  const std::string& name = args[1];
  if (!validName(name)) {
    throw std::logic_error("bad name");
  }
  if (info.set.count(name) || info.db.count(name)) {
    throw std::logic_error("name exists");
  }
  const long long size_ll = parseNumber(args[2]);
  if (size_ll <= 0 || static_cast< size_t >(size_ll) != args.size() - min_args) {
    throw std::logic_error("bad set size");
  }
  const size_t size = static_cast< size_t >(size_ll);
  std::vector< shape_t > v;
  v.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    auto it = info.db.find(args[min_args + i]);
    if (it == info.db.end()) {
      throw std::logic_error("shape not found");
    }
    v.push_back(it->second);
  }
  info.set[name] = std::move(v);
}

void hvostov::showSet(std::ostream& out, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 2;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  auto it = info.set.find(args[1]);
  if (it == info.set.end()) {
    throw std::logic_error("set not found");
  }
  for (const shape_t& s : it->second) {
    out << s << "\n";
  }
}

void hvostov::getFrame(std::ostream& out, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 2;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  auto it = info.db.find(args[1]);
  if (it == info.db.end()) {
    throw std::logic_error("shape not found");
  }
  out << calculateFrame(it->second) << "\n";
}

void hvostov::getFrameSet(std::ostream& out, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 2;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  auto it = info.set.find(args[1]);
  if (it == info.set.end()) {
    throw std::logic_error("set not found");
  }
  out << calculateFrame(it->second) << "\n";
}

void hvostov::spawnProcess(std::ostream&, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 3;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  const std::string& name = args[1];
  if (!validName(name)) {
    throw std::logic_error("bad name");
  }
  if (info.procs.count(name)) {
    throw std::logic_error("process exists");
  }
  const long long seed_ll = parseNumber(args[2]);
  if (seed_ll < 0) {
    throw std::logic_error("bad seed");
  }

  const size_t seed = static_cast< size_t >(seed_ll);
  const std::string shm_name = "/lab2_" + name + "_" + std::to_string(getpid());
  auto shm = std::make_shared< ShmGuard >(shm_name, sizeof(ShmLayout), true);

  int to_child[2] = {-1, -1};
  int from_child[2] = {-1, -1};
  if (pipe(to_child) != 0 || pipe(from_child) != 0) {
    throw std::system_error(errno, std::generic_category(), "pipe");
  }

  FdGuard tc_r(to_child[0]);
  FdGuard tc_w(to_child[1]);
  FdGuard fc_r(from_child[0]);
  FdGuard fc_w(from_child[1]);

  const pid_t pid = fork();
  if (pid < 0) {
    throw std::system_error(errno, std::generic_category(), "fork");
  }

  if (pid == 0) {
    tc_w.close();
    fc_r.close();
    const int rc = runChild(seed, tc_r.get(), fc_w.get(), reinterpret_cast< ShmLayout* >(shm->get()));
    _exit(rc);
  }

  tc_r.close();
  fc_w.close();

  process_t p{};
  p.pid = pid;
  p.to_child = tc_w.release();
  p.from_child = fc_r.release();
  p.shm = std::move(shm);
  p.seed = seed;
  info.procs[name] = std::move(p);
}

void hvostov::areaOn(std::ostream&, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 6;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  const std::string& proc_name = args[1];
  const std::string& calc_name = args[2];
  const std::string& set_name = args[3];
  if (!validName(calc_name)) {
    throw std::logic_error("bad calc name");
  }
  if (info.procs.find(proc_name) == info.procs.end()) {
    throw std::logic_error("process not found");
  }
  if (info.calc.count(calc_name)) {
    throw std::logic_error("calc exists");
  }
  if (info.set.find(set_name) == info.set.end()) {
    throw std::logic_error("set not found");
  }
  const long long threads_ll = parseNumber(args[4]);
  const long long tries_ll = parseNumber(args[5]);
  if (threads_ll <= 0 || tries_ll <= 0) {
    throw std::logic_error("bad threads or tries");
  }

  process_t& p = info.procs[proc_name];
  if (p.pending.size() >= MAX_SLOTS) {
    throw std::logic_error("too many pending");
  }

  const std::vector< shape_t >& shapes = info.set[set_name];
  if (shapes.empty() || shapes.size() > MAX_SHAPES) {
    throw std::logic_error("bad shape count");
  }

  ShmLayout* const layout = reinterpret_cast< ShmLayout* >(p.shm->get());
  const size_t slot_idx = layout->write_idx % MAX_SLOTS;
  ShmSlot& slot = layout->slots[slot_idx];

  std::snprintf(slot.req.calc_name, sizeof(slot.req.calc_name), "%s", calc_name.c_str());
  slot.req.threads = static_cast< int >(threads_ll);
  slot.req.tries = static_cast< size_t >(tries_ll);
  slot.req.shape_count = shapes.size();
  slot.req.seed = p.seed;
  std::copy(shapes.begin(), shapes.end(), slot.shapes);
  layout->write_idx++;

  int err = 0;
  const char sig = 1;
  if (send(err, p.to_child, &sig, 1) != 1) {
    throw std::system_error(errno, std::generic_category(), "send signal");
  }

  p.pending.push_back(calc_name);
  info.calc[calc_name] = {proc_name, false, false, 0.0, 0.0};
}

void hvostov::status(std::ostream& out, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 2;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  auto it = info.calc.find(args[1]);
  if (it == info.calc.end()) {
    throw std::logic_error("calc not found");
  }
  const calc_state_t& st = it->second;
  if (!st.done && !st.canceled) {
    auto proc_it = info.procs.find(st.process_name);
    if (proc_it != info.procs.end()) {
      tryReadResult(info, proc_it->second);
    }
  }
  out << info.calc[args[1]] << "\n";
}

void hvostov::wait(std::ostream& out, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 2;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  auto it = info.calc.find(args[1]);
  if (it == info.calc.end()) {
    throw std::logic_error("calc not found");
  }
  const calc_state_t& st = it->second;
  if (st.canceled) {
    out << st << "\n";
    return;
  }
  if (!st.done) {
    auto proc_it = info.procs.find(st.process_name);
    if (proc_it == info.procs.end()) {
      throw std::logic_error("process not found");
    }
    process_t& p = proc_it->second;
    while (!info.calc[args[1]].done) {
      if (p.pending.empty()) {
        throw std::logic_error("calc not pending");
      }
      if (tryReadResult(info, p)) {
        continue;
      }
      char sig = 0;
      int err = 0;
      if (recv(err, p.from_child, &sig, 1) != 1) {
        throw std::runtime_error("read failed");
      }
      const ShmLayout* const layout = reinterpret_cast< const ShmLayout* >(p.shm->get());
      const size_t idx = p.res_idx % MAX_SLOTS;
      const ShmSlot& slot = layout->slots[idx];
      const std::string cn(slot.res.calc_name, strnlen(slot.res.calc_name, sizeof(slot.res.calc_name)));
      auto cit = info.calc.find(cn);
      if (cit != info.calc.end()) {
        cit->second.done = true;
        cit->second.result = slot.res.result;
        cit->second.time_ms = slot.res.time_ms;
      }
      p.res_idx++;
      auto pit = std::find(p.pending.begin(), p.pending.end(), cn);
      if (pit != p.pending.end()) {
        p.pending.erase(pit);
      }
    }
  }
  out << info.calc[args[1]] << "\n";
}

void hvostov::timeout(std::ostream& out, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 3;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  auto it = info.calc.find(args[1]);
  if (it == info.calc.end()) {
    throw std::logic_error("calc not found");
  }
  const long long ms_ll = parseNumber(args[2]);
  if (ms_ll < 0) {
    throw std::logic_error("bad timeout");
  }
  const calc_state_t& st = it->second;
  if (st.canceled) {
    out << st << "\n";
    return;
  }
  if (!st.done) {
    auto proc_it = info.procs.find(st.process_name);
    if (proc_it != info.procs.end()) {
      process_t& p = proc_it->second;
      struct pollfd pfd = {p.from_child, POLLIN, 0};
      if (poll(&pfd, 1, static_cast< int >(ms_ll)) > 0) {
        char sig = 0;
        int err = 0;
        if (recv(err, p.from_child, &sig, 1) == 1) {
          const ShmLayout* const layout = reinterpret_cast< const ShmLayout* >(p.shm->get());
          const size_t idx = p.res_idx % MAX_SLOTS;
          const ShmSlot& slot = layout->slots[idx];
          const std::string cn(slot.res.calc_name, strnlen(slot.res.calc_name, sizeof(slot.res.calc_name)));
          auto cit = info.calc.find(cn);
          if (cit != info.calc.end()) {
            cit->second.done = true;
            cit->second.result = slot.res.result;
            cit->second.time_ms = slot.res.time_ms;
          }
          p.res_idx++;
          auto pit = std::find(p.pending.begin(), p.pending.end(), cn);
          if (pit != p.pending.end()) {
            p.pending.erase(pit);
          }
        }
      }
    }
  }
  out << info.calc[args[1]] << "\n";
}

void hvostov::killProcess(std::ostream&, const Args& args, info_t& info)
{
  constexpr size_t arg_count = 2;
  if (args.size() != arg_count) {
    throw std::logic_error("bad args count");
  }
  auto it = info.procs.find(args[1]);
  if (it == info.procs.end()) {
    throw std::logic_error("process not found");
  }
  process_t& p = it->second;
  for (const std::string& calc_name : p.pending) {
    auto cit = info.calc.find(calc_name);
    if (cit != info.calc.end()) {
      cit->second.canceled = true;
    }
  }
  p.pending.clear();
  if (p.pid > 0) {
    ::kill(p.pid, SIGTERM);
    int status = 0;
    waitpid(p.pid, &status, 0);
    p.pid = -1;
  }
  if (p.to_child >= 0) {
    close(p.to_child);
    p.to_child = -1;
  }
  if (p.from_child >= 0) {
    close(p.from_child);
    p.from_child = -1;
  }
  info.procs.erase(it);
}
