#include <iostream>

int main() {
  char8_t pa[] = {0x7F, 0x92, 0x13, 0x01, 0x20, 0x00};
  std::cout << strchr((const char*)pa, 0X81);
  return 0;
}
