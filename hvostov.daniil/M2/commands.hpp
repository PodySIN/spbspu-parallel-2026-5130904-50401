#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "types.hpp"
#include "shm.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <unistd.h>

namespace hvostov {
  using Args = std::vector< std::string >;

  struct process_t {
    pid_t pid = -1;
    int to_child = -1;
    int from_child = -1;
    std::shared_ptr< ShmGuard > shm;
    std::vector< std::string > pending;
    size_t seed = 0;
    size_t res_idx = 0;
  };

  struct info_t {
    std::unordered_map< std::string, shape_t > db;
    std::unordered_map< std::string, std::vector< shape_t > > set;
    std::unordered_map< std::string, process_t > procs;
    std::unordered_map< std::string, calc_state_t > calc;
  };

  using cmd_t = std::function< void(std::ostream&, const Args&, info_t&) >;

  Args splitLine(const std::string& line);

  void addCircle(std::ostream& out, const Args& args, info_t& info);
  void addEllipse(std::ostream& out, const Args& args, info_t& info);
  void rotate(std::ostream& out, const Args& args, info_t& info);
  void show(std::ostream& out, const Args& args, info_t& info);
  void set(std::ostream& out, const Args& args, info_t& info);
  void showSet(std::ostream& out, const Args& args, info_t& info);
  void getFrame(std::ostream& out, const Args& args, info_t& info);
  void getFrameSet(std::ostream& out, const Args& args, info_t& info);
  void spawnProcess(std::ostream& out, const Args& args, info_t& info);
  void areaOn(std::ostream& out, const Args& args, info_t& info);
  void status(std::ostream& out, const Args& args, info_t& info);
  void wait(std::ostream& out, const Args& args, info_t& info);
  void timeout(std::ostream& out, const Args& args, info_t& info);
  void killProcess(std::ostream& out, const Args& args, info_t& info);
}

#endif
