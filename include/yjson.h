#ifndef YJSON_H
#define YJSON_H

#include <ctype.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using namespace std::literals;

class YJson final {
 private:
  static constexpr char8_t utf8bom[] = {0xEF, 0xBB, 0xBF};
  // static constexpr char8_t utf16le[] = {0xFF, 0xFE};

  static constexpr char16_t utf16FirstWcharMark[3] = {0xD800, 0xDC00, 0xE000};
  static constexpr char8_t utf8FirstCharMark[7] = {0x00, 0x00, 0xC0, 0xE0,
                                                   0xF0, 0xF8, 0xFC};

 public:
  explicit YJson(): _type(Null) {}
  enum Type { False = 0, True = 1, Null, Number, String, Array, Object };
  enum Encode { UTF8, UTF8BOM };
  typedef std::pair<std::u8string, YJson> ObjectItemType;
  typedef std::list<ObjectItemType> ObjectType;
  typedef ObjectType::iterator ObjectIterator;
  typedef ObjectType::const_iterator ObjectConstIterator;
  typedef YJson ArrayItemType;
  typedef std::list<ArrayItemType> ArrayType;
  typedef ArrayType::iterator ArrayIterator;
  typedef ArrayType::const_iterator ArrayConstIterator;

 private:
  // typedef std::u8string_view::const_iterator StrIterator;

  template <typename StrIterator>
  static StrIterator StrSkip(StrIterator first, StrIterator last) {
    while (first != last && static_cast<char8_t>(*first) <= 32)
      first++;
    return first;
  }

 public:
  YJson(Type type) : _type(type) {
    switch (type) {
      case YJson::Object:
        _value.Object = new ObjectType;
        break;
      case YJson::Array:
        _value.Array = new ArrayType;
        break;
      case YJson::String:
        _value.String = new std::u8string;
        break;
      case YJson::Number:
        _value.Double = new double(0);
      default:
        break;
    }
  }
  YJson(double val) : _type(YJson::Number) {
    _value.Double = new double(val);
  }
  YJson(int val) : YJson(static_cast<double>(val)) {}
  YJson(std::u8string str) : _type(YJson::String) {
    _value.String = new std::u8string(std::move(str));
  }
  YJson(std::u8string_view str) : _type(YJson::String) {
    _value.String = new std::u8string(str.begin(), str.end());
  }
  YJson(const char8_t* str) : YJson(std::u8string_view(str)) {}
  YJson(bool val) : _type(val ? YJson::True : YJson::False) {}
  YJson(std::nullptr_t) : _type(YJson::Null) {}
  YJson(ArrayType array) : _type(YJson::Array) {
    _value.Array = new ArrayType(std::move(array));
  }
  YJson(ObjectType object) : _type(YJson::Object) {
    _value.Object = new ObjectType(std::move(object));
  }
  YJson(const YJson& other) : _type(other._type) {
    switch (_type) {
      case YJson::Array:
        _value.Array = new ArrayType(other._value.Array->begin(),
                                     other._value.Array->end());
        break;
      case YJson::Object:
        _value.Object = new ObjectType(other._value.Object->begin(),
                                       other._value.Object->end());
        break;
      case YJson::String:
        _value.String = new std::u8string(*other._value.String);
        break;
      case YJson::Number:
        _value.Double = new double(*other._value.Double);
        break;
      default:
        break;
    }
  }

  YJson(YJson&& other) noexcept
    : _type(other._type)
  {
    _value = other._value;
    // other._value = nullptr;
    other._type = YJson::Null;
  }

  template <typename _Iterator>
  YJson(_Iterator first, _Iterator last): YJson() {
    if (first >= last) {
      throw std::logic_error("YJson Error: The iterator range is wrong.");
    }
    parseValue(StrSkip(first, last), last);
  }
  YJson(const char8_t* first, size_t size): YJson(first, first + size) {}

  typedef std::initializer_list<std::pair<std::u8string_view, YJson>> O;
  YJson(YJson::O lst) : _type(YJson::Type::Object) {
    _value.Object = new ObjectType;
    for (auto& [i, j] : lst) {
      _value.Object->emplace_back(i, j);
    }
  }

  typedef std::initializer_list<YJson> A;
  YJson(YJson::A lst) : _type(YJson::Array) {
    _value.Array = new ArrayType;
    for (auto& i : lst) {
      _value.Array->emplace_back(i);
    }
  }

  explicit YJson(const std::filesystem::path& path, Encode encode);
  ~YJson();

  YJson::Type& getType() { return _type; }
  const YJson::Type& getType() const { return const_cast<YJson*>(this)->getType(); }

  std::u8string& getValueString() { return *_value.String; }
  const std::u8string& getValueString() const { return const_cast<YJson*>(this)->getValueString(); }
  template<typename _Ty=int32_t>
  _Ty getValueInt() const { return static_cast<_Ty>(*_value.Double); }
  double& getValueDouble() { return *_value.Double; }
  const double& getValueDouble() const { return const_cast<YJson*>(this)->getValueDouble(); }
  ObjectType& getObject() { return *_value.Object; }
  const ObjectType& getObject() const { return const_cast<YJson*>(this)->getObject(); }
  ArrayType& getArray() { return *_value.Array; }
  const ArrayType& getArray() const { return const_cast<YJson*>(this)->getArray(); }

  static unsigned char _toHex(unsigned char x) {
    constexpr int A = 'A' - 10;
    return x > 9 ? (x + A) : x + '0';
  }

  static unsigned char _fromHex(unsigned char x) {
    unsigned char y;
    if (x >= 'A' && x <= 'Z')
      y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z')
      y = x - 'a' + 10;
    else if (x >= '0' && x <= '9')
      y = x - '0';
    else
      throw std::logic_error("YJson Error: Hex error.");
    return y;
  }

  void popBackA() { _value.Array->pop_back(); }

  void popBackO() { _value.Object->pop_back(); }

  template <typename _Iterator>
  void assignA(_Iterator first, _Iterator last) {
    if (_type != YJson::Array) {
      clearData();
      _type = YJson::Array;
      _value.Array = new ArrayType;
    }
    _value.Array->assign(first, last);
  }

  template <typename _CharT>
  static std::basic_string<_CharT> pureUrlEncode(
      const std::basic_string_view<_CharT> str) {
    std::basic_string_view<_CharT> str_view(str.begin(),
                                            str.begin() + str.length());
    std::basic_string<_CharT> ret;
    ret.reserve(str_view.size());
    for (auto i : str_view) {
      if (std::isalnum(static_cast<char8_t>(i)) || u8"-_.~"sv.find(i) != std::u8string_view::npos)
        ret.push_back(i);
      // else if (i == ' ')
      //     ret.push_back('+');
      else {
        ret.push_back('%');
        ret.push_back(_toHex(static_cast<char8_t>(i) >> 4));
        ret.push_back(_toHex(static_cast<char8_t>(i) % 16));
      }
    }
    return ret;
  }

  template <typename _Ty = std::u8string>
  static std::u8string pureUrlDecode(const std::u8string& str) {
    std::u8string_view str_view(str.begin(), str.begin() + str.length());
    std::u8string ret(str_view.size(), 0);
    for (size_t i = 0; i < str.length(); i++) {
      if (str[i] == '+')
        ret.push_back(' ');
      else if (str[i] == '%') {
        if (i + 2 < str.length())
          throw std::logic_error("YJson Error: Url decode error.");
        unsigned char high = _fromHex((unsigned char)str[++i]);
        unsigned char low = _fromHex((unsigned char)str[++i]);
        ret.push_back((high << 4) | low);
      } else
        ret.push_back(str[i]);
    }
    return ret;
  }

  std::u8string urlEncode() const;
  std::u8string urlEncode(const std::u8string_view url) const;

  size_t sizeA() const { return _value.Array->size(); }
  size_t sizeO() const { return _value.Object->size(); }

  std::u8string toString(bool fmt = false) const {
    std::ostringstream result;
    if (fmt) {
      printValue(result, 0);
    } else {
      printValue(result);
    }
    const std::string str = result.str();
    return std::u8string(str.begin(), str.end());
  }

  bool toFile(const std::filesystem::path& file_name,
                     bool fmt = true,
                     const Encode& encode = UTF8) {
    std::ofstream result(file_name, std::ios::out | std::ios::binary);
    if (!result.is_open()) {
      return false;
    }
    if (encode == UTF8BOM) {
      result.write(reinterpret_cast<const char*>(utf8bom), 3);
    }
    if (fmt) {
      printValue(result, 0);
    } else {
      printValue(result);
    }
    result.close();
    return true;
  }

  YJson& operator=(const YJson& other) {
    if (this == &other)
      return *this;
    clearData();
    _type = other._type;
    switch (_type) {
      case YJson::Array:
        _value.Array = new ArrayType(other._value.Array->begin(),
                                     other._value.Array->end());
        break;
      case YJson::Object:
        _value.Object = new ObjectType(other._value.Object->begin(),
                                       other._value.Object->end());
        break;
      case YJson::String:
        _value.String = new std::u8string(*other._value.String);
        break;
      case YJson::Number:
        _value.Double = new double(*other._value.Double);
        break;
      default:
        break;
    }
    return *this;
  }

  YJson& operator=(YJson&& other) noexcept {
    YJson::swap(*this, other);
    return *this;
  }

  YJson& operator=(std::u8string str) {
    clearData();
    _type = YJson::String;
    _value.String = new std::u8string(std::move(str));
    return *this;
  }

  YJson& operator=(std::u8string_view str) {
    std::u8string data(str);
    return this->operator=(data);
  }

  YJson& operator=(const char8_t* str) {
    return this->operator=(std::u8string_view(str));
  }

  YJson& operator=(double val) {
    clearData();
    _type = YJson::Number;
    _value.Double = new double(val);
    return *this;
  }

  YJson& operator=(int val) {
    return this->operator=(static_cast<double>(val));
  }

  YJson& operator=(bool val) {
    clearData();
    _type = static_cast<YJson::Type>(val);
    return *this;
  }

  YJson& operator=(std::nullptr_t) {
    clearData();
    _type = YJson::Null;
    return *this;
  }

  YJson& operator=(YJson::Type type) {
    clearData();
    switch (_type = type) {
      case YJson::Object:
        _value.Object = new ObjectType;
        break;
      case YJson::Array:
        _value.Array = new ArrayType;
        break;
      case YJson::String:
        _value.String = new std::u8string;
        break;
      case YJson::Number:
        _value.Double = new double(0);
      default:
        break;
    }

    return *this;
  }

  ArrayItemType& operator[](size_t i) { return *find(i); }
  const ArrayItemType& operator[](size_t i) const { return const_cast<YJson*>(this)->operator[](i); }

  ArrayItemType& operator[](int i) { return const_cast<YJson*>(this)->operator[](static_cast<size_t>(i)); }
  const ArrayItemType& operator[](int i) const { return const_cast<YJson*>(this)->operator[](i); }

  ArrayItemType& operator[](const char8_t* key) {
    auto itr = find(key);
    if (itr == _value.Object->end()) {
      return _value.Object->emplace(itr, key, YJson::Null)->second;
    } else {
      return itr->second;
    }
  }

  const ArrayItemType& operator[](const char8_t* key) const {
    return const_cast<YJson *>(this)->operator[](key);
  }

  ArrayItemType& operator[](const std::u8string_view key) {
    auto itr = find(key);
    if (itr == _value.Object->end()) {
      return _value.Object->emplace(itr, key, YJson::Null)->second;
    } else {
      return itr->second;
    }
  }
  const ArrayItemType& operator[](const std::u8string_view key) const {
    return const_cast<YJson*>(this)->operator[](key);
  }

  bool operator==(const YJson& other) const {
    if (this == &other)
      return true;
    if (_type != other._type)
      return false;
    switch (_type) {
      case YJson::Array:
        return *_value.Array == *other._value.Array;
      case YJson::Object:
        return *_value.Object == *other._value.Object;
      case YJson::Number:
        return *_value.Double == *other._value.Double;
      case YJson::String:
        return *_value.String == *other._value.String;
      case YJson::Null:
      case YJson::False:
      case YJson::True:
      default:
        return true;
    }
  }

  bool operator==(bool val) const {
    return _type == static_cast<YJson::Type>(val);
  }
  bool operator==(std::nullptr_t) const {
    return _type == YJson::Null;
  }
  bool operator==(int val) const {
    if (_type != YJson::Number)
      return false;
    return fabs((double)val - *_value.Double) <=
           std::numeric_limits<double>::epsilon();
  }
  bool operator!=(int val) const {
    if (_type != YJson::Number)
      return true;
    return fabs(val - *_value.Double) > std::numeric_limits<double>::epsilon();
  }
  bool operator==(const std::u8string_view str) const {
    return _type == YJson::String && *_value.String == str;
  }
  bool operator==(const std::u8string& str) const {
    return _type == YJson::String && *_value.String == str;
  }
  bool operator==(const char8_t* str) const {
    return _type == YJson::String && *_value.String == str;
  }

  void setText(std::u8string val) {
    if (_type != YJson::String) {
      clearData();
      _type = YJson::String;
      _value.String = new std::u8string(std::move(val));
    } else {
      _value.String->swap(val);
    }
  }

  template <typename _Iterator>
  void setText(_Iterator first, _Iterator last) {
    if (_type != YJson::String) {
      clearData();
      _type = YJson::String;
      _value.String = new std::u8string(first, last);
    } else {
      _value.String->assign(first, last);
    }
  }

  void setText(const std::filesystem::path& val) {
    if (_type != YJson::String) {
      clearData();
      _type = YJson::String;
      _value.String = new std::u8string(val.u8string());
    } else {
      _value.String->assign(val.u8string());
    }
  }

  template <typename _Ty>
  void setText(const _Ty& utf8Array) {
    if (_type != YJson::String) {
      clearData();
      _type = YJson::String;
    } else {
      delete _value.String;
    }
    _value.String = new std::u8string(utf8Array.begin(), utf8Array.end());
  }

  void setValue(double val) {
    clearData();
    _type = YJson::Number;
    _value.Double = new double(val);
  }

  void setValue(int val) { setValue(static_cast<double>(val)); }

  void setValue(bool val) {
    clearData();
    _value.Void = nullptr;
    _type = val ? True : False;
  }

  void setNull() {
    clearData();
    _value.Void = nullptr;
    _type = YJson::Null;
  }

  YJson& joinA(const YJson& js);
  static YJson joinA(YJson j1, const YJson& j2) { return j1.joinA(j2); }
  YJson& joinO(const YJson& js);
  static YJson joinO(YJson j1, const YJson& j2) { return j1.joinO(j2); }
  YJson& join(const YJson& js);
  static YJson join(YJson j1, const YJson& j2) { return j1.join(j2); }

  ArrayIterator find(size_t index) {
    auto iter = _value.Array->begin();
    while (index-- && iter != _value.Array->end())
      ++iter;
    return iter;
  }
  const ArrayIterator find(size_t index) const {
    auto iter = _value.Array->begin();
    while (index-- && iter != _value.Array->end())
      ++iter;
    return iter;
  }
  ArrayIterator findByValA(int value) {
    return findByValA(static_cast<double>(value));
  }
  const ArrayIterator findByValA(int value) const {
    return findByValA(static_cast<double>(value));
  }
  ArrayIterator findByValA(double value) {
    return std::find_if(_value.Array->begin(), _value.Array->end(),
                        [value](const YJson& item) {
                          return item._type == YJson::Number &&
                                 fabs(value - *item._value.Double) <=
                                     std::numeric_limits<double>::epsilon();
                        });
  }
  const ArrayIterator findByValA(double value) const {
    return std::find_if(_value.Array->begin(), _value.Array->end(),
                        [value](const YJson& item) {
                          return item._type == YJson::Number &&
                                 fabs(value - *item._value.Double) <=
                                     std::numeric_limits<double>::epsilon();
                        });
  }
  ArrayIterator findByValA(const std::u8string_view str) {
    return std::find(_value.Array->begin(), _value.Array->end(), str);
  }
  const ArrayIterator findByValA(const std::u8string_view str) const {
    return std::find(_value.Array->begin(), _value.Array->end(), str);
  }

  template <typename _Ty = const std::u8string_view>
  ArrayIterator append(_Ty value) {
    return _value.Array->emplace(_value.Array->end(), value);
  }

  ArrayIterator append(YJson&& value) {
    return _value.Array->emplace(_value.Array->end(), std::move(value));
  }

  ArrayIterator remove(ArrayIterator item) { return _value.Array->erase(item); }
  ArrayIterator removeA(size_t index) { return remove(find(index)); }
  template <typename _Ty>
  ArrayIterator removeByValA(_Ty str) {
    auto iter = findByValA(str);
    return (iter != _value.Array->end()) ? remove(iter) : iter;
  }

  ObjectIterator find(const std::u8string_view key) {
    return std::find_if(_value.Object->begin(), _value.Object->end(),
                        [&key](const YJson::ObjectItemType& item) {
                          return item.first == key;
                        });
  }
  const ObjectIterator find(const std::u8string_view key) const {
    return std::find_if(_value.Object->begin(), _value.Object->end(),
                        [&key](const YJson::ObjectItemType& item) {
                          return item.first == key;
                        });
  }
  ObjectIterator find(const char8_t* key) {
    return find(std::u8string_view(key));
  }
  const ObjectIterator find(const char8_t* key) const {
    return find(std::u8string_view(key));
  }
  ObjectIterator findByValO(double value) {
    return std::find_if(_value.Object->begin(), _value.Object->end(),
                        [&value](const YJson::ObjectItemType& item) {
                          return item.second._type == YJson::Number &&
                                 fabs(value - *item.second._value.Double) <=
                                     std::numeric_limits<double>::epsilon();
                        });
  }
  ObjectIterator findByValO(const std::u8string_view str) {
    return std::find_if(_value.Object->begin(), _value.Object->end(),
                        [&str](const YJson::ObjectItemType& item) {
                          return item.second._type == YJson::String &&
                                 *item.second._value.String == str;
                        });
  }
  ObjectIterator findByValO(int value) {
    return findByValO(static_cast<double>(value));
  }

  template <typename _Ty = const std::u8string_view>
  ObjectIterator append(_Ty value, const std::u8string_view key) {
    return _value.Object->emplace(_value.Object->end(), key, value);
  }

  ObjectIterator remove(const std::u8string_view key) {
    return remove(find(key));
  }
  ObjectIterator remove(const char8_t* key) { return remove(find(key)); }
  ObjectIterator remove(ObjectIterator item) {
    return _value.Object->erase(item);
  }
  template <typename _Ty>
  ObjectIterator removeByValO(_Ty str) {
    auto iter = findByValO(str);
    return (iter != _value.Object->end()) ? remove(iter) : iter;
  }

  static bool isUtf8BomFile(const std::filesystem::path& path);

  static void swap(YJson& A, YJson& B) {
    std::swap(A._type, B._type);
    std::swap(A._value, B._value);
  }
  void swap(YJson& other) {
    std::swap(_type, other._type);
    std::swap(_value, other._value);
  }

  bool isArray() const { return _type == Array; }
  bool isObject() const { return _type == Object; }
  bool isString() const { return _type == String; }
  bool isNumber() const { return _type == Number; }
  bool isTrue() const { return _type == True; }
  bool isFalse() const { return _type == False; }
  bool isNull() const { return _type == Null; }

  bool emptyA() const { return _value.Array->empty(); }
  bool emptyO() const { return _value.Object->empty(); }

  void clearA() { _value.Array->clear(); }
  void clearO() { _value.Object->clear(); }
  ArrayIterator beginA() { return _value.Array->begin(); }
  ObjectIterator beginO() { return _value.Object->begin(); }
  ArrayIterator endA() { return _value.Array->end(); }
  ObjectIterator endO() { return _value.Object->end(); }
  ObjectItemType& frontO() { return _value.Object->front(); }
  ObjectItemType& backO() { return _value.Object->back(); }
  ArrayItemType& frontA() { return _value.Array->front(); }
  ArrayItemType& backA() { return _value.Array->back(); }

  ArrayConstIterator beginA() const { return _value.Array->begin(); }
  ObjectConstIterator beginO() const { return _value.Object->begin(); }
  ArrayConstIterator endA() const { return _value.Array->end(); }
  ObjectConstIterator endO() const { return _value.Object->end(); }
  const ObjectItemType frontO() const { return _value.Object->front(); }
  const ObjectItemType backO() const { return _value.Object->back(); }
  const ArrayItemType frontA() const { return _value.Array->front(); }
  const ArrayItemType backA() const { return _value.Array->back(); }

  friend std::ostream& operator<<(std::ofstream& out, const YJson& outJson);
  friend std::ostream& operator<<(std::ostream& out, const YJson& outJson);

 private:
  YJson::Type _type;
  union JsonValue {
    void* Void;
    double* Double = nullptr;
    std::u8string* String;
    ObjectType* Object;
    ArrayType* Array;
  } _value;

  template <typename StrIterator>
  StrIterator parseValue(StrIterator first, StrIterator last) {
    StrIterator iter = first;
    if (last == first)
      goto empty;

    if (*first == '\"') {
      std::u8string buffer;
      iter = parseString(buffer, first, last);
      _type = YJson::String;
      _value.String = new std::u8string(std::move(buffer));
    } else if (*first == '-' || (*first >= '0' && *first <= '9')) {
      double buffer;
      iter = parseNumber(first, last, buffer);
      _type = YJson::Number;
      _value.Double = new double { buffer };
    } else if (*first == '[') {
      ArrayType buffer;
      iter = parseArray(first, last, buffer);
      _type = YJson::Array;
      _value.Array = new ArrayType(std::move(buffer));
    } else if (*first == '{') {
      ObjectType buffer;
      iter = parseObject(first, last, buffer);
      _type = YJson::Object;
      _value.Object = new ObjectType(std::move(buffer));
    } else {
      iter += 4;
      if (iter > last) goto empty;

      if (std::equal(first, iter, "null")) {
        _type = YJson::Null;
      } else if (std::equal(first, iter, "true")) {
        _type = YJson::True;
      } else if (++iter <= last && std::equal(first, iter, "false")) {
        _type = YJson::False;
      } else {
        goto empty;
      }
    }
    return iter;
empty:
    throw std::runtime_error("YJson Error: Parse empty data!");
  }

  template <typename StrIterator>
  static StrIterator parseNumber(StrIterator first, StrIterator last, double& buffer) {
    buffer = 0;
    int sign = 1;
    int scale = 0;
    int signsubscale = 1, subscale = 0;

    if (*first == '-') {
      sign = -1;
      if (++first == last) {
        goto invalid;
      }
    }
    if (*first == '0') {
      return ++first;
    }

    if (isdigit(*first)) {
      do {
        buffer *= 10;
        buffer += *first - '0';
      } while (++first != last && isdigit(*first));

      if (first == last) {
        return first;
      }
    }

    if (*first == '.') {
      while (++first != last && isdigit(*first)) {
        buffer *= 10.0;
        buffer += *first - '0';
        scale--;
      }

      if (first == last) {
        goto result;
      }
    }

    if ('e' == *first || 'E' == *first) {
      if (++first == last) {
        goto invalid;
      }
      if (*first == '-') {
        signsubscale = -1;
        ++first;
      } else if (*first == '+') {
        ++first;
      }
      for (;first != last && isdigit(*first);++first) {
        subscale *= 10;
        subscale += *first - '0';
      }
    }
result:
    buffer *= sign * pow(10, scale + signsubscale * subscale);
    return first;
invalid:
    throw std::runtime_error("YJson Error: Invalid Number.");
  }

  template <typename T>
  static void parseHex4(T c, uint16_t& h) {
    if (c >= '0' && c <= '9')
      h += c - '0';
    else if (c >= 'A' && c <= 'F')
      h += 10 + c - 'A';
    else if (c >= 'a' && c <= 'f')
      h += 10 + c - 'a';
    else
      throw std::runtime_error("YJson Error: Invalid hexadecimal sequence!");
  }

  template <typename T>
  static uint16_t parseHex4(T& str) {
    uint16_t h = 0;
    parseHex4(*++str, h);
    parseHex4(*++str, h = h << 4);
    parseHex4(*++str, h = h << 4);
    parseHex4(*++str, h = h << 4);
    return h;
  }

  template <typename StrIterator>
  static StrIterator parseString(std::u8string& des,
                                 StrIterator first,
                                 StrIterator last) {
    char8_t bufferBegin[4], *bufferEnd;
    size_t len;
    des.clear();
    StrIterator ptr;
    char32_t uc, uc2;
    for (ptr = first + 1; ptr != last && *ptr != '\"'; ++ptr) {
      if (*ptr != '\\') {
        des.push_back(*ptr);
        continue;
      }
      if (++ptr == last) {
        throw std::runtime_error("YJson Error: String's length was too short to be parsed.");
      }
      switch (*ptr) {
        case 'b':
          des.push_back('\b');
          break;
        case 'f':
          des.push_back('\f');
          break;
        case 'n':
          des.push_back('\n');
          break;
        case 'r':
          des.push_back('\r');
          break;
        case 't':
          des.push_back('\t');
          break;
        case 'u': // like \uAABB
          if (ptr + 5 > last) {
            goto error_throw;
          }
          uc = parseHex4(ptr);

          // Single wide character.

          // Two wide characters.
          if (uc >= utf16FirstWcharMark[0] && uc < utf16FirstWcharMark[1]) {
            // Multipile wide characters.
            if (++ptr + 6 > last) {
              goto error_throw;
            }
            if (*ptr != '\\' || *++ptr != 'u')
              throw std::runtime_error("YJson Error: Missing second-half of surrogate.");
            uc2 = parseHex4(ptr);
            if (uc2 >= utf16FirstWcharMark[1] && uc2 < utf16FirstWcharMark[2]) {
              uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
            } else {
              throw std::runtime_error("YJson Error: Invalid second-half of surrogate.");
            }
          }

          len = 4;
          if (uc < 0x80)
            len = 1;
          else if (uc < 0x800)
            len = 2;
          else if (uc < 0x10000)
            len = 3;
          else if (uc > 0x10FFFF) {
            throw std::runtime_error("YJson Error: Invalid Unicode.");
          }
          bufferEnd = bufferBegin + len;

          switch (len) {
            case 4:
              *--bufferEnd = ((uc | 0x80) & 0xBF);
              uc >>= 6;
              [[fallthrough]];
            case 3:
              *--bufferEnd = ((uc | 0x80) & 0xBF);
              uc >>= 6;
              [[fallthrough]];
            case 2:
              *--bufferEnd = ((uc | 0x80) & 0xBF);
              uc >>= 6;
              [[fallthrough]];
            case 1:
              *--bufferEnd = (uc | utf8FirstCharMark[len]);
          }
          des.append(bufferBegin, bufferBegin + len);
          break;
error_throw:
          throw std::runtime_error("YJson Error: Unicode chars are too short.");
        default:
          des.push_back(*ptr);
          break;
      }
    }
    if (ptr == last || *ptr != '"') {
      throw std::runtime_error("YJson Error: String missing right quotes.");
    }
    return ++ptr;
  }

  template <typename StrIterator>
  static StrIterator parseArray(StrIterator first, StrIterator last, ArrayType& buffer) {
    first = StrSkip(++first, last);
    if (first == last) {
      goto missing;
    }
    if (*first == ']') {
      return ++first;
    }

    buffer.emplace_back();
    first = StrSkip(buffer.back().parseValue(first, last), last);
    if (first == last)
      goto missing;

    for (decltype(first) iter; *first == ',';) {
      iter = StrSkip(++first, last);
      if (iter == last) {
        goto missing;
      }
      if (*iter == ']') {
        return ++iter;
      }
      buffer.emplace_back();
      first = StrSkip(buffer.back().parseValue(iter, last), last);
      if (first == last) {
        goto missing;
      }
    }

    first = StrSkip(first, last);
    if (*first == ']')
      return ++first;
missing:
    throw std::runtime_error("YJson Error: Array missing right square brackets.");
  }

  template <typename StrIterator>
  static StrIterator parseObject(StrIterator first, StrIterator last, ObjectType& buffer) {
    first = StrSkip(++first, last);
    if (first == last) {
      goto missing;
    }
    if (*first == '}') {
      return ++first;
    }

    buffer.emplace_back();
    first = StrSkip(parseString(buffer.back().first, first, last), last);

    if (first == last) {
      goto missing;
    }
    if (*first != ':') {
      goto invalid;
    }

    first = StrSkip(++first, last);
    first = StrSkip(buffer.back().second.parseValue(first, last), last);

    if (first == last) {
      goto missing;
    }
    
    for (decltype(first) iter; *first == ','; ) {
      iter = StrSkip(++first, last);
      if (iter == last) {
        goto missing;
      }

      if (*iter == '}') {
        return ++iter;
      }
      buffer.emplace_back();
      first = StrSkip(parseString(buffer.back().first, iter, last), last);

      if (*first != ':') {
        goto invalid;
      }
      first = StrSkip(buffer.back().second.parseValue(StrSkip(++first, last), last), last);
      if (first == last) {
        goto missing;
      }
    }

    if (*first == '}') {
      return ++first;
    }
missing:
    throw std::runtime_error("YJson Error: Object missing right brace.");
invalid:
    throw std::runtime_error("YJson Error: Invalid Object.");
  }

  template <typename _Ty>
  void printValue(_Ty& pre) const {
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
        printArray(pre);
        break;
      case YJson::Object:
        return printObject(pre);
      default:
        throw std::runtime_error("YJson Error: Unknown type to print.");
        return;
    }
  }
  void printValue(std::ostream& pre, int depth) const;
  void printNumber(std::ostream& pre) const {
    pre << std::format("{}", *_value.Double);
  }
  static void printString(std::ostream& pre, const std::u8string_view str);
  void printArray(std::ostream& pre) const;
  void printArray(std::ostream& pre, int depth) const;
  void printObject(std::ostream& pre) const;
  void printObject(std::ostream& pre, int depth) const;

  void clearData() {
    switch (_type) {
      case YJson::Object:
        delete _value.Object;
        break;
      case YJson::Array:
        delete _value.Array;
        break;
      case YJson::Number:
        delete _value.Double;
      default:
        break;
    }
  }
};


#endif
