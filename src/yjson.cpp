#include <yjson/yjson.h>

#include <cassert>
#include <iomanip>
#include <stdexcept>

constexpr std::array<char8_t, 3> YJson::utf8bom;
// constexpr char8_t YJson::utf16le[];

constexpr std::array<char16_t, 3> YJson::utf16FirstWcharMark;
constexpr std::array<char8_t, 7> YJson::utf8FirstCharMark;

YJson::YJson(const std::filesystem::path& path, YJson::Encode encode): _type(Null) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("YJson Error: File does not exist.");
  }
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);
  switch (encode) {
    case YJson::UTF8BOM: {
      if (size < 3) {
        throw std::runtime_error("YJson Error: File does not begin with UTF-8 BOM.");
      }
      size -= 3;
      std::u8string json_string(size, u8'\0');
      file.seekg(3, std::ios::beg);
      file.read(reinterpret_cast<char *>(json_string.data()), size);
      file.close();
      auto first = StrSkip(json_string.begin(), json_string.end());
      parseValue(first, json_string.end());
      break;
    }
    case YJson::UTF8: {
      std::u8string json_string(size, u8'\0');
      file.read(reinterpret_cast<char *>(json_string.data()), size);
      file.close();
      auto first = StrSkip(json_string.begin(), json_string.end());
      parseValue(first, json_string.end());
      break;
    }
    default:
      throw std::runtime_error("YJson Error: File encoding format not supported.");
  }
}

std::u8string YJson::urlEncode() const {
  if (_type != YJson::Object) {
    throw std::logic_error("YJson Error: YJson instance type is not YJson::Object.");
  }
  std::ostringstream param;
  for (const auto& [key, value] : *_value.Object) {
    param.write(reinterpret_cast<const char*>(key.data()), key.size());
    param.put('=');
    switch (value._type) {
      case YJson::Number:
        value.printNumber(param);
        break;
      case YJson::String: {
        std::u8string str = pureUrlEncode<char8_t>(*value._value.String);
        param.write(reinterpret_cast<const char*>(str.data()), str.size());
        break;
      }
      case YJson::True:
        param.write("true", 4);
        break;
      case YJson::False:
        param.write("false", 5);
        break;
      case YJson::Null:
      default:
        param.write("null", 4);
    }
    param.put('&');
  }
  auto&& result = param.str();
  if (!result.empty())
    result.pop_back();
  return std::u8string(result.begin(), result.end());
}

std::u8string YJson::urlEncode(const std::u8string_view url) const {
  if (_type != YJson::Object) {
    throw std::logic_error("YJson Error: YJson instance type is not YJson::Object.");
  }
  std::ostringstream param;
  param.write(reinterpret_cast<const char*>(url.data()), url.size());
  for (const auto& [key, value] : *_value.Object) {
    param.write(reinterpret_cast<const char*>(key.data()), key.size());
    param.put('=');
    switch (value._type) {
      case YJson::Number:
        value.printNumber(param);
        break;
      case YJson::String: {
        auto str = pureUrlEncode<char8_t>(*value._value.String);
        param.write(reinterpret_cast<const char*>(str.data()), str.size());
        break;
      }
      case YJson::True:
        param.write("true", 4);
        break;
      case YJson::False:
        param.write("false", 5);
        break;
      case YJson::Null:
      default:
        param.write("null", 4);
    }
    param.put('&');
  }
  auto&& result = param.str();
  if (!result.empty())
    result.pop_back();
  return std::u8string(result.begin(), result.end());
}

bool YJson::isUtf8BomFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (file.is_open()) {
    std::array<char8_t, 3> bom;
    if (file.read(reinterpret_cast<char*>(bom.data()), 3)) {
      if (bom == utf8bom) {
        file.close();
        return true;
      }
    } else {
      file.close();
    }
  }
  return false;
}

YJson& YJson::joinA(const YJson& js) {
  assert(isArray() && js.isArray());
  if (&js == this)
    return joinA(YJson(*this));
  _value.Array->insert(_value.Array->end(), js._value.Array->begin(), js._value.Array->end());
  return *this;
}

YJson& YJson::joinO(const YJson& js) {
  assert(isObject() && js.isObject());
  if (&js == this)
    return joinO(YJson(*this));
  _value.Object->insert(_value.Object->end(), js._value.Object->begin(), js._value.Object->end());
  return *this;
}

YJson& YJson::join(const YJson& js) {
  return isArray() ? joinA(js) : joinO(js);
}

void YJson::printValue(std::ostream& pre, int depth) const {
  switch (_type) {
    case YJson::Null:
      pre.write("null", 4);
      break;
    case YJson::False:
      pre.write("false", 5);
      break;
    case YJson::True:
      pre.write("true", 4);
      break;
    case YJson::Number:
      printNumber(pre);
      break;
    case YJson::String:
      printString(pre, *_value.String);
      break;
    case YJson::Array:
      printArray(pre, depth);
      break;
    case YJson::Object:
      printObject(pre, depth);
      break;
    default:
      throw std::runtime_error("YJson Error: Unknown yjson type.");
  }
}

void YJson::printString(std::ostream& pre, const std::u8string_view str) {
  pre.put('\"');
  if (str.empty()) {
    pre.put('\"');
    return;
  }

  constexpr auto cmp = [](char8_t c) -> bool {
    return c < 32 || c == u8'\"' || c == u8'\\';
  };

  if (std::find_if(str.begin(), str.end(), cmp) == str.end()) {
    pre.write(reinterpret_cast<const char*>(str.data()), str.size());
    pre.put('\"');
    return;
  }

  for (const auto c : str) {
    if (cmp(c)) {
      pre.put('\\');
      switch (c) {
        case '\\':
          pre.put('\\');
          break;
        case '\"':
          pre.put('\"');
          break;
        case '\b':
          pre.put('b');
          break;
        case '\f':
          pre.put('f');
          break;
        case '\n':
          pre.put('n');
          break;
        case '\r':
          pre.put('r');
          break;
        case '\t':
          pre.put('t');
          break;
        default:
          pre.put('u');
          pre << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<uint16_t>(c) << std::setbase(10);
          break;
      }
    } else {
      pre.put(c);
    }
  }
  pre.put('\"');
}

void YJson::printArray(std::ostream& pre) const {
  if (_value.Array->empty()) {
    pre.write("[]", 2);
    return;
  }
  pre.put('[');
  auto i = _value.Array->begin(), j = _value.Array->end();
  for (--j; i != j; ++i) {
    i->printValue(pre);
    pre.put(',');
  }
  i->printValue(pre);
  pre.put(']');
}

void YJson::printArray(std::ostream& pre, int depth) const {
  constexpr int depthTimes = 2;
  if (_value.Array->empty()) {
    pre.write("[]", 2);
    return;
  }
  ++depth;
  pre.write("[\n", 2);
  auto i = _value.Array->begin(), j = _value.Array->end();
  for (--j; i != j; ++i) {
    pre << std::string(depth << depthTimes, ' ');
    i->printValue(pre, depth);
    pre.write(",\n", 2);
  }
  pre << std::string(depth << depthTimes, ' ');
  i->printValue(pre, depth);
  pre.put('\n');
  pre << std::string(--depth << depthTimes, ' ');
  pre.put(']');
}

void YJson::printObject(std::ostream& pre) const {
  if (_value.Object->empty()) {
    pre.write("{}", 2);
    return;
  }
  pre.put('{');
  auto i = _value.Object->begin(), j = _value.Object->end();
  for (--j; j != i; ++i) {
    printString(pre, i->first);
    pre.put(':');
    i->second.printValue(pre);
    pre.put(',');
  }
  printString(pre, i->first);
  pre.put(':');
  i->second.printValue(pre);
  pre.put('}');
}

void YJson::printObject(std::ostream& pre, int depth) const {
  constexpr int depthTimes = 2;
  if (_value.Object->empty()) {
    pre.write("{}", 2);
    return;
  }
  ++depth;
  pre.write("{\n", 2);
  auto i = _value.Object->begin(), j = _value.Object->end();
  for (--j; i != j; ++i) {
    pre << std::string(depth << depthTimes, ' ');
    printString(pre, i->first);
    pre.write(": ", 2);
    i->second.printValue(pre, depth);
    pre.write(",\n", 2);
  }
  pre << std::string(depth << depthTimes, ' ');
  printString(pre, i->first);
  pre.write(": ", 2);
  i->second.printValue(pre, depth);
  pre.put('\n');
  pre << std::string(--depth << depthTimes, ' ');
  pre.put('}');
}
std::ostream& operator<<(std::ofstream& out, const YJson& outJson) {
  outJson.printValue(out, 0);
  return out << std::endl;
}

std::ostream& operator<<(std::ostream& out, const YJson& outJson) {
  outJson.printValue(out, 0);
  return out << std::endl;
}
