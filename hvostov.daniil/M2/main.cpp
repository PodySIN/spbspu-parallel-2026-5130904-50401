#include "commands.hpp"
#include <iostream>
#include <string>
#include <unordered_map>

int main()
{
  std::unordered_map< std::string, hvostov::cmd_t > cmds;
  hvostov::info_t info;
  cmds["circle"] = hvostov::addCircle;
  cmds["ellipse"] = hvostov::addEllipse;
  cmds["rotate"] = hvostov::rotate;
  cmds["show"] = hvostov::show;
  cmds["set"] = hvostov::set;
  cmds["showset"] = hvostov::showSet;
  cmds["frame"] = hvostov::getFrame;
  cmds["frameset"] = hvostov::getFrameSet;
  cmds["spawn"] = hvostov::spawnProcess;
  cmds["area-on"] = hvostov::areaOn;
  cmds["status"] = hvostov::status;
  cmds["wait"] = hvostov::wait;
  cmds["timeout"] = hvostov::timeout;
  cmds["kill"] = hvostov::killProcess;

  std::string line;
  while (std::getline(std::cin, line)) {
    const hvostov::Args args = hvostov::splitLine(line);
    if (args.empty()) {
      continue;
    }
    auto it = cmds.find(args[0]);
    if (it == cmds.end()) {
      std::cout << "<UNKNOWN COMMAND>\n";
      continue;
    }
    try {
      it->second(std::cout, args, info);
    } catch (...) {
      std::cout << "<INVALID COMMAND>\n";
    }
  }
  return 0;
}
