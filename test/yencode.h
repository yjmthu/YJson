#ifndef YENCODE_H
#define YENCODE_H

#include <cstring>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace YEncode {

constexpr uint16_t utf16FirstWcharMark[3] = {0xD800, 0xDC00, 0xE000};
constexpr unsigned char utf8FirstCharMark[7] = {0x00, 0x00, 0xC0, 0xE0,
                                                0xF0, 0xF8, 0xFC};
constexpr uint32_t get_0x1s(const int i) {
  return (1 << (8 - i)) - 1;
}

template <class C1>
bool utf8_to_utf16LE(C1 utf16, const char* utf8) {
  uint32_t unicode;
  uint8_t len;  // bool double_wchar = false;
  do {
    unicode = static_cast<unsigned char>(*utf8);
    if (unicode < 0x80)
      len = 1;
    else if (unicode >> 6 == 0x2)
      return false;
    else if (unicode < utf8FirstCharMark[3])
      len = 2;
    else if (unicode < utf8FirstCharMark[4])
      len = 3;
    else if (unicode < utf8FirstCharMark[5])
      len = 4;
    else if (unicode < utf8FirstCharMark[6])
      len = 5;
    else
      len = 6;
    unicode &= get_0x1s(len);
    for (auto i = len; --i;) {
      unicode <<= 6;
      unicode |= *(++utf8) & 0x3F;
    }
    if (len <= 3) {
      utf16.push_back(unicode);
    } else {
      utf16.push_back(utf16FirstWcharMark[0] | (unicode >> 10));
      utf16.push_back(utf16FirstWcharMark[1] | (unicode & 0x3FF));
    }
  } while (*++utf8);
  utf16.push_back(0);
  return true;
}

template <class C1>
bool utf8_to_utf16LE(C1 utf16, const std::u8string& utf8) {
  uint32_t unicode;
  uint8_t len;
  std::u8string::const_iterator iter = utf8.begin();
  do {
    unicode = static_cast<unsigned char>(*iter);
    if (unicode < 0x80)
      len = 1;
    else if (unicode >> 6 == 0x2)
      return false;
    else if (unicode < utf8FirstCharMark[3])
      len = 2;
    else if (unicode < utf8FirstCharMark[4])
      len = 3;
    else if (unicode < utf8FirstCharMark[5])
      len = 4;
    else if (unicode < utf8FirstCharMark[6])
      len = 5;
    else
      len = 6;
    unicode &= get_0x1s(len);
    for (auto i = len; --i;) {
      unicode <<= 6;
      unicode |= *(++iter) & 0x3F;
    }
    if (len <= 3) {
      utf16.push_back(unicode);
    } else {
      utf16.push_back(utf16FirstWcharMark[0] | (unicode >> 10));
      utf16.push_back(utf16FirstWcharMark[1] | (unicode & 0x3FF));
    }
  } while (++iter != utf8.end());
  utf16.push_back(0);
  return true;
}

template <typename C1, typename C2>
bool utf16LE_to_utf8(C1 utf8, C2 utf16) {
  uint32_t unicode;
  uint8_t len;
  do {
    unicode = *utf16;
    if (unicode < 0x80)
      len = 1;  // 6位以内，1字节
    else if (unicode < 0x800)
      len = 2;  // 11位以内，2字节
    else if (unicode >= utf16FirstWcharMark[0] &&
             unicode < utf16FirstWcharMark[1])  // 双宽字节，对应4~6字节
    {
      if (*++utf16 < utf16FirstWcharMark[1] || *utf16 >= utf16FirstWcharMark[2])
        return false;
      unicode = ((unicode & 0x3FF) << 10) | (*utf16 & 0x3FF);
      if (unicode < 0x200000)
        len = 4;
      else if (unicode < 0x4000000)
        len = 5;
      else
        len = 6;
    } else if (unicode >= utf16FirstWcharMark[1] &&
               unicode < utf16FirstWcharMark[2])  // 双宽字节第二位，无效
      return false;
    else
      len = 3;  // 16位以内，3字节
    utf8.resize(utf8.size() + len);
    auto ptr = utf8.end();
    for (auto i = len; --i;) {
      *--ptr = 0x80 | (unicode & 0x3F);
      unicode >>= 6;
    }
    *--ptr = utf8FirstCharMark[len] | unicode;
  } while (*++utf16);
  utf8.push_back(0);
  return true;
}

}  // namespace YEncode

#endif  // YENCODE_H
