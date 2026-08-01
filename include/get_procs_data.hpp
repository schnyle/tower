#pragma once

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct ProcData
{
  int pid;
  std::string name;
};

inline std::vector<ProcData> get_procs_data()
{
  int pid;
  std::string name;
  std::vector<ProcData> result;

  for (const auto &entry : std::filesystem::directory_iterator("/proc"))
  {
    std::string_view filename = entry.path().c_str();
    filename.remove_prefix(filename.rfind('/') + 1);

    auto [ptr, ec] = std::from_chars(filename.begin(), filename.end(), pid);
    if (ec != std::errc{} || ptr != filename.data() + filename.size())
    {
      continue;
    }

    std::ifstream file(entry.path() / "comm");
    if (!file)
    {
      continue;
    }

    if (!std::getline(file, name))
    {
      continue;
    }

    // std::cout << entry.path() << " - " << name << '\n';
    result.push_back({pid, name});
  }

  return result;
};
