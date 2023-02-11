#ifndef YJSON_H
#define YJSON_H

#include <filesystem>
#include <format>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

class YJson final {
 private:
  static constexpr char8_t utf8bom[] = {0xEF, 0xBB, 0xBF};
  // static constexpr char8_t utf16le[] = {0xFF, 0xFE};

  static constexpr char16_t utf16FirstWcharMark[3] = {0xD800, 0xDC00, 0xE000};
  static constexpr char8_t utf8FirstCharMark[7] = {0x00, 0x00, 0xC0, 0xE0,
                                                   0xF0, 0xF8, 0xFC};

 public:
  explicit YJson() {}
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
    while (first < last && static_cast<char8_t>(*first) <= 32)
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
  YJson(int val) : _type(YJson::Number) {
    _value.Double = new double(val);
  }
  YJson(const std::u8string_view str) : _type(YJson::String) {
    _value.String = new std::u8string(str);
  }
  YJson(const std::u8string& str) : _type(YJson::String) {
    _value.String = new std::u8string(str);
  }
  YJson(const char8_t* str) : _type(YJson::String) {
    _value.String = new std::u8string(str);
  }
  YJson(const std::filesystem::path& str) : _type(YJson::String) {
    _value.String = new std::u8string(str.u8string());
  }
  YJson(bool val) : _type(val ? YJson::True : YJson::False) {}
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

  YJson(YJson&& other)
    : _type(other._type)
  {
    _value = other._value;
    // other._value = nullptr;
    other._type = YJson::Null;
  }

  template <typename _Iterator>
  YJson(_Iterator first, _Iterator last) {
    parseValue(StrSkip(first, last), last);
  }

  YJson(const char8_t* first, size_t size) {
    parseValue(StrSkip(first, first + size), first + size);
  }

  typedef std::initializer_list<std::pair<const std::u8string_view, YJson>> O;
  YJson(YJson::O lst) : _type(YJson::Type::Object) {
    _value.Object = new ObjectType;
    for (auto& [i, j] : lst) {
      _value.Object->emplace_back(i, std::move(j));
    }
  }

  typedef std::initializer_list<YJson> A;
  YJson(YJson::A lst) : _type(YJson::Array) {
    _value.Array = new ArrayType;
    for (auto& i : lst) {
      _value.Array->emplace_back(std::move(i));
    }
  }

  explicit YJson(const std::filesystem::path& path, Encode encode);
  ~YJson();

  YJson::Type getType() const { return _type; }
  bool isSameType(const YJson* other) const {
    if (!other) return false;
    if (_type == True || _type == False)
      return other->_type == True || other->_type == False;
    return _type == other->_type;
  }

  std::u8string& getValueString() { return *_value.String; }
  const std::u8string& getValueString() const { return *_value.String; }
  int32_t getValueInt() { return static_cast<int32_t>(*_value.Double); }
  const int32_t getValueInt() const {
    return static_cast<int32_t>(*_value.Double);
  }
  double& getValueDouble() { return *_value.Double; }
  double getValueDouble() const { return *_value.Double; }
  ObjectType& getObject() { return *_value.Object; }
  const ObjectType& getObject() const { return *_value.Object; }
  ArrayType& getArray() { return *_value.Array; }
  const ArrayType& getArray() const { return *_value.Array; }

  static unsigned char _toHex(unsigned char x) {
    return x > 9 ? x + 55 : x + 48;
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
      throw 0;
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
      if (isalnum(static_cast<char8_t>(i)) || strchr("-_.~", i))
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
        ret += ' ';
      else if (str[i] == '%') {
        if (i + 2 < str.length())
          throw nullptr;
        unsigned char high = _fromHex((unsigned char)str[++i]);
        unsigned char low = _fromHex((unsigned char)str[++i]);
        ret.push_back(high * 16 + low);
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

  YJson& operator=(const std::u8string_view str) {
    clearData();
    _type = YJson::String;
    _value.String = new std::u8string(str);
    return *this;
  }

  YJson& operator=(std::u8string&& str) {
    clearData();
    _type = YJson::String;
    _value.String = new std::u8string(std::move(str));
    return *this;
  }

  YJson& operator=(const std::u8string& str) {
    clearData();
    _type = YJson::String;
    _value.String = new std::u8string(str);
    return *this;
  }

  YJson& operator=(const char8_t* str) {
    clearData();
    _type = YJson::String;
    _value.String = new std::u8string(str);
    return *this;
  }

  YJson& operator=(double val) {
    clearData();
    _type = YJson::Number;
    _value.Double = new double(val);
    return *this;
  }

  YJson& operator=(int val) {
    clearData();
    _type = YJson::Number;
    _value.Double = new double(val);
    return *this;
  }

  YJson& operator=(bool val) {
    clearData();
    _type = static_cast<YJson::Type>(val);
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
  const ArrayItemType& operator[](size_t i) const { return *find(i); }

  ArrayItemType& operator[](int i) { return *find(i); }
  const ArrayItemType& operator[](int i) const { return *find(i); }

  ArrayItemType& operator[](const char8_t* key) {
    auto itr = find(key);
    if (itr == _value.Object->end()) {
      return _value.Object->emplace(itr, key, YJson::Null)->second;
    } else {
      return itr->second;
    }
  }

  const ArrayItemType& operator[](const char8_t* key) const {
    auto itr = find(key);
    if (itr == _value.Object->end()) {
      return _value.Object->emplace(itr, key, YJson::Null)->second;
    } else {
      return itr->second;
    }
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
    auto itr = find(key);
    if (itr == _value.Object->end()) {
      return _value.Object->emplace(itr, key, YJson::Null)->second;
    } else {
      return itr->second;
    }
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

  void setText(const std::u8string_view val) {
    if (_type != YJson::String) {
      clearData();
      _type = YJson::String;
      _value.String = new std::u8string(val);
    } else {
      _value.String->assign(val);
    }
  }

  void setText(const char8_t* val) {
    if (_type != YJson::String) {
      clearData();
      _type = YJson::String;
      _value.String = new std::u8string(val);
    } else {
      _value.String->assign(val);
    }
  }

  void setText(const std::u8string& val) {
    if (_type != YJson::String) {
      clearData();
      _type = YJson::String;
      _value.String = new std::u8string(val);
    } else {
      _value.String->assign(val);
    }
  }

  void setText(std::u8string&& val) {
    if (_type != YJson::String) {
      clearData();
      _type = YJson::String;
      _value.String = new std::u8string;
    }
    _value.String->swap(val);
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

  bool joinA(const YJson&);
  static YJson joinA(const YJson& j1, const YJson& j2) {
    return YJson(j1).joinA(j2);
  }
  bool joinO(const YJson&);
  static YJson joinO(const YJson& j1, const YJson& j2) {
    return YJson(j1).joinO(j2);
  }

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
    if (last <= first)
      return last;
    if (*first == '\"') {
      _type = YJson::String;
      _value.String = new std::u8string;
      return parseString(*_value.String, first, last);
    }
    if (*first == '-' || (*first >= '0' && *first <= '9')) {
      return parseNumber(first, last);
    }
    if (*first == '[') {
      return parseArray(first, last);
    }
    if (*first == '{') {
      return parseObject(first, last);
    }
    if (std::equal(first, first + 4, "null")) {
      _type = YJson::Null;
      return first + 4;
    }
    if (std::equal(first, first + 5, "false")) {
      _type = YJson::False;
      return first + 5;
    }
    if (std::equal(first, first + 4, "true")) {
      _type = YJson::True;
      return first + 4;
    }
    return last;
  }

  template <typename StrIterator>
  StrIterator parseNumber(StrIterator first, StrIterator) {
    _type = YJson::Number;
    _value.Double = new double(0);
    short sign = 1;
    int scale = 0;
    int signsubscale = 1, subscale = 0;
    if (*first == '-') {
      sign = -1;
      ++first;
    }
    if (*first == '0') {
      return ++first;
    }
    if ('1' <= *first && *first <= '9')
      do
        *_value.Double += (*_value.Double *= 10.0, *first++ - '0');
      while (isdigit(*first));
    if ('.' == *first && isdigit(first[1])) {
      ++first;
      do
        *_value.Double += (*_value.Double *= 10.0, *first++ - '0'), scale--;
      while (isdigit(*first));
    }
    if ('e' == *first || 'E' == *first) {
      if (*++first == '-') {
        signsubscale = -1;
        ++first;
      } else if (*first == '+') {
        ++first;
      }
      while (isdigit(*first)) {
        subscale *= 10;
        subscale += *first++ - '0';
      }
    }
    *_value.Double *= sign * pow(10, scale + signsubscale * subscale);
    return first;
  }

  template <typename T>
  static bool _parse_hex4(T str, uint16_t& h) {
    if (*str >= '0' && *str <= '9')
      h += (*str) - '0';
    else if (*str >= 'A' && *str <= 'F')
      h += 10 + (*str) - 'A';
    else if (*str >= 'a' && *str <= 'f')
      h += 10 + (*str) - 'a';
    else
      return true;
    return false;
  }

  template <typename T>
  static uint16_t parse_hex4(T str) {
    uint16_t h = 0;
    if (_parse_hex4(str, h) || _parse_hex4(++str, h = h << 4) ||
        _parse_hex4(++str, h = h << 4) || _parse_hex4(++str, h = h << 4))
      return 0;
    return h;
  }

  template <typename StrIterator>
  static StrIterator parseString(std::u8string& des,
                                 StrIterator first,
                                 StrIterator last) {
    char bufferBegin[4], *bufferEnd;
    size_t len;
    des.clear();
    StrIterator ptr;
    char32_t uc, uc2;
    for (ptr = first + 1; ptr != last && *ptr != '\"'; ++ptr) {
      if (*ptr != '\\') {
        des.push_back(*ptr);
        continue;
      }
      switch (*++ptr) {
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
        case 'u':
          uc = parse_hex4(ptr + 1);
          ptr += 4; /* get the unicode char. */

          if ((uc >= utf16FirstWcharMark[1] && uc < utf16FirstWcharMark[2]) ||
              uc == 0)
            break; /* check for invalid.    */

          if (uc >= utf16FirstWcharMark[0] &&
              uc < utf16FirstWcharMark[1]) /* UTF16 surrogate pairs.    */
          {
            if (ptr[1] != '\\' || ptr[2] != 'u')
              break; /* missing second-half of surrogate.    */
            uc2 = parse_hex4(ptr + 3);
            ptr += 6;
            if (uc2 < utf16FirstWcharMark[1] || uc2 >= utf16FirstWcharMark[2])
              break; /* invalid second-half of surrogate.    */
            uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
          }

          len = 4;
          if (uc < 0x80)
            len = 1;
          else if (uc < 0x800)
            len = 2;
          else if (uc < 0x10000)
            len = 3;
          bufferEnd = bufferBegin + len;

          switch (len) {
            case 4:
              *--bufferEnd = ((uc | 0x80) & 0xBF);
              uc >>= 6;
            case 3:
              *--bufferEnd = ((uc | 0x80) & 0xBF);
              uc >>= 6;
            case 2:
              *--bufferEnd = ((uc | 0x80) & 0xBF);
              uc >>= 6;
            case 1:
              *--bufferEnd = (uc | utf8FirstCharMark[len]);
          }
          des.append(bufferBegin, bufferBegin + len);
          break;
        default:
          des.push_back(*ptr);
          break;
      }
    }
    return ++ptr;
  }

  template <typename StrIterator>
  StrIterator parseArray(StrIterator first, StrIterator last) {
    _type = YJson::Array;
    auto& array = *(_value.Array = new ArrayType);
    first = StrSkip(++first, last);
    if (*first == ']') {
      return first + 1;
    }
    array.emplace_back();
    first = StrSkip(array.back().parseValue(StrSkip(first, last), last), last);
    if (first >= last)
      return last;

    while (*first == ',') {
      if (*StrSkip(first + 1, last) == ']') {
        return StrSkip(first + 1, last);
      }
      array.emplace_back();
      first = StrSkip(array.back().parseValue(StrSkip(first + 1, last), last),
                      last);
      if (first >= last)
        return last;
    }

    if (*first++ == ']')
      return first;
    throw std::u8string(u8"Can't find the ']'!");
  }

  template <typename StrIterator>
  StrIterator parseObject(StrIterator first, StrIterator last) {
    _type = YJson::Object;
    ObjectType& object = *(_value.Object = new ObjectType);
    first = StrSkip(++first, last);
    if (*first == '}')
      return first + 1;
    object.emplace_back();
    first = StrSkip(
        parseString(object.back().first, StrSkip(first, last), last), last);
    if (first >= last)
      return last;
    if (*first != ':') {
      throw std::runtime_error("Invalid Object.");
    }
    first = StrSkip(
        object.back().second.parseValue(StrSkip(first + 1, last), last), last);
    while (*first == ',') {
      if (*StrSkip(first + 1, last) == '}') {
        return StrSkip(first + 1, last);
      }
      object.emplace_back();
      first = StrSkip(
          parseString(object.back().first, StrSkip(first + 1, last), last),
          last);
      if (*first != ':') {
        throw std::runtime_error("Invalid Object.");
      }
      first = StrSkip(
          object.back().second.parseValue(StrSkip(first + 1, last), last),
          last);
      if (!(*first))
        return last;
    }

    if (*first == '}') {
      return first + 1;
    }
    throw std::runtime_error("Invalid Object.");
  }

  template <typename _Ty>
  void printValue(_Ty& pre) const {
    using namespace std::literals;
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
        std::cerr << "yjson: unknown type to print.\n";
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

std::ostream& operator<<(std::ofstream& out, const YJson& outJson) {
  outJson.printValue(out, 0);
  return out << std::endl;
}

std::ostream& operator<<(std::ostream& out, const YJson& outJson) {
  outJson.printValue(out, 0);
  return out << std::endl;
}

#endif
