#include <iostream>
#include <stdexcept>
#include <string.h>
#include <yjson.h>

#include <filesystem>
#include <Windows.h>

namespace fs = std::filesystem;
int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "No argv was found.\n";
    return 0;
  }

  SetConsoleOutputCP(CP_UTF8);

  fs::path json_path = argv[1];
#if 0
  try {
    YJson json(json_path, YJson::UTF8);
    std::cout << json;
  } catch (std::runtime_error er) {
    SetConsoleOutputCP(CP_ACP);
    std::cout << er.what() << std::endl;
  }
#else
  YJson json(json_path, YJson::UTF8);
  std::cout << json;
#endif
  return 0;
}
