#include <iostream>
#include <stdexcept>
#include <string.h>
#include <yjson/yjson.h>

#include <filesystem>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace fs = std::filesystem;
int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "No argv was found.\n";
    return 0;
  }

#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif

  fs::path json_path = argv[1];
#if 0
  try {
    YJson json(json_path, YJson::UTF8);
    std::cout << json;
  } catch (const std::runtime_error& er) {
    SetConsoleOutputCP(CP_ACP);
    std::cout << er.what() << std::endl;
  }
#else
  YJson json(json_path, YJson::UTF8);
  std::cout << json;
#endif
  return 0;
}
