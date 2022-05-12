#ifndef YJSON_H
#define YJSON_H

#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>
#include <list>
#include <limits>
#include <string_view>
#include <initializer_list>
#include <system_error>
#include <iostream>

class YJson final
{
public:
    inline explicit YJson() { }
    enum Type { False=0, True=1, Null, Number, String, Array, Object };
    enum Encode { UTF8, UTF8BOM, UTF16LE, UTF16LEBOM, UTF16BE, UTF16BEBOM };
    typedef std::pair<std::string, YJson> ObjectItemType;
    typedef std::list<ObjectItemType> ObjectType;
    typedef ObjectType::iterator ObjectIterator;
    typedef YJson ArrayItemType;
    typedef std::list<ArrayItemType> ArrayType;
    typedef ArrayType::iterator ArrayIterator;

    inline YJson(Type type): _type(type) {
        switch (type) {
        case YJson::Object:
            _value.Object = new ObjectType;
            break;
        case YJson::Array:
            _value.Array = new ArrayType;
            break;
        case YJson::String:
            _value.String = new std::string;
            break;
        case YJson::Number:
            _value.Double = new double(0);
        default:
            break;
        }
    }
    inline YJson(double val): _type(YJson::Number) { _value.Double = new double(val); }
    inline YJson(int val): _type(YJson::Number) { _value.Double = new double(val); }
    inline YJson(const std::string_view str): _type(YJson::String) { _value.String = new std::string(str); }
    inline YJson(const char* str): _type(YJson::String) { _value.String = new std::string(str); }
    inline YJson(bool val): _type(val? YJson::True: YJson::False) {}
    YJson(const YJson& other): _type(other._type) {
        switch (_type) {
        case YJson::Array:
            _value.Array = new ArrayType(other._value.Array->begin(), other._value.Array->end());
            break;
        case YJson::Object:
            _value.Object = new ObjectType(other._value.Object->begin(), other._value.Object->end());
            break;
        case YJson::String:
            _value.String = new std::string(*other._value.String);
            break;
        case YJson::Number:
            _value.Double = new double(*other._value.Double);
            break;
        default:
            break;
        }
    }
    explicit YJson(const std::string& path, Encode encode);
    ~YJson();

    inline YJson::Type getType() const { return _type; }

    inline const std::string& getValueString() const { return *_value.String; }
    inline int getValueInt() { return *_value.Double; }
    inline double getValueDouble() { return *_value.Double; }
    inline ObjectType& getObject() { return *_value.Object; }
    inline ArrayType& getArray() { return *_value.Array; }
    std::string urlEncode() const;
    std::string urlEncode(const std::string_view url) const;

    inline int sizeA() const { return _value.Array->size();}
    inline int sizeO() const { return _value.Object->size(); }

    inline std::string toString(bool fmt=false) const {
        std::string result;
        if (fmt) {
            printValue(result, 0);
        } else {
            printValue(result);
        }
        return result;
    }
    bool toFile(const std::string& name, const YJson::Encode& encode=UTF8, bool fmt=true);

    YJson& operator=(const YJson& other) {
        clearData();
        _type = other._type;
        switch (_type) {
        case YJson::Array:
            _value.Array = new ArrayType(other._value.Array->begin(), other._value.Array->end());
            break;
        case YJson::Object:
            _value.Object = new ObjectType(other._value.Object->begin(), other._value.Object->end());
            break;
        case YJson::String:
            _value.String = new std::string(*other._value.String);
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

    inline ArrayItemType& operator[](int i) { return *find(i); }
    inline ObjectItemType& operator[](const char* key) { return *find(key); }
    inline ObjectItemType& operator[](const std::string_view key) { return *find(key); }
    inline bool operator==(const YJson& other) const {
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
    bool operator==(bool val) const { return _type == static_cast<YJson::Type>(val); }
    bool operator==(const std::string_view str) const { return _type == YJson::String && *_value.String == str; }
    bool operator==(const char* str) const { return _type == YJson::String && *_value.String == str; }

    inline void setText(const std::string_view val) {
        clearData();
        _type = YJson::String;
        _value.String = new std::string(val);
    }
    inline void setText(const char* val) {
        clearData();
        _type = YJson::String;
        _value.String = new std::string(val);
    }
    inline void setValue(double val) {
        clearData();
        _type = YJson::Number;
        _value.Double = new double(val);
    }
    inline void setValue(int val) { setValue(static_cast<double>(val)); }
    inline void setValue(bool val) {
        clearData();
        _value.Void = nullptr;
        _type = val? True: False;
    }
    inline void setNull() {
        clearData();
        _value.Void = nullptr;
        _type = YJson::Null;
    }

    bool joinA(const YJson&);
    static YJson joinA(const YJson& j1, const YJson& j2) {
        return YJson(j1).joinA(j2);
    }
    bool joinO(const YJson&);
    static inline YJson joinO(const YJson& j1, const YJson& j2) {
        return YJson(j1).joinO(j2);
    }

    inline ArrayIterator find(size_t index) {
        auto iter = _value.Array->begin();
        while (index-- && iter != _value.Array->end()) ++iter;
        return iter;
    }
    inline ArrayIterator findByValA(int value) {
        return findByValA(static_cast<double>(value));
    }
    ArrayIterator findByValA(double value) {
        return std::find_if(_value.Array->begin(), _value.Array->end(), [value](const YJson& item){
            return item._type == YJson::Number && fabs(value-*item._value.Double) <= std::numeric_limits<double>::epsilon();
        });
    }
    ArrayIterator findByValA(const std::string_view str) {
        return std::find(_value.Array->begin(), _value.Array->end(), str);
    }

    template<typename _Ty=const std::string_view>
    inline ArrayIterator append(_Ty value) {
        return _value.Array->emplace(_value.Array->end(), value);
    }
    
    ArrayIterator removeA(ArrayIterator item) {
        return _value.Array->erase(item);
    }
    inline ArrayIterator removeA(size_t index) {
        return removeA(find(index));
    }
    inline ArrayIterator removeByValA(int value) {
        return removeA(findByValA(value));
    }
    inline ArrayIterator removeByValA(double value) {
        return removeA(findByValA(value));
    }
    inline ArrayIterator removeByValA(const std::string_view str) { return removeA(findByValA(str)); }

    inline ObjectIterator find(const std::string_view key) {
        return std::find_if(_value.Object->begin(), _value.Object->end(), [&key](const YJson::ObjectItemType& item){ return item.first == key; });
    }
    inline ObjectIterator find(const char* key) {
        return find(std::string_view(key));
    }
    inline ObjectIterator findByValO(double value) {
        return std::find_if(_value.Object->begin(), _value.Object->end(), [&value](const YJson::ObjectItemType& item){
            return item.second._type == YJson::Number && fabs(value-*item.second._value.Double) <= std::numeric_limits<double>::epsilon();
        });
    }
    inline ObjectIterator findByValO(const std::string_view str) {
        return std::find_if(_value.Object->begin(), _value.Object->end(), [&str](const YJson::ObjectItemType& item){ return item.second._type == YJson::String && *item.second._value.String == str; });
    }
    inline ObjectIterator findByValO(int value) {
        return findByValO(static_cast<double>(value));
    }

    template<typename _Ty=const std::string_view>
    inline ObjectIterator append(_Ty value, const std::string_view key) {
        return _value.Object->emplace(_value.Object->end(), key, value);
    }
    
    inline ObjectIterator removeO(const std::string_view key) {
        return removeO(find(key));
    }
    inline ObjectIterator removeO(const char* key) {
        return removeO(find(key));
    }
    inline ObjectIterator removeO(ObjectIterator item) {
        return _value.Object->erase(item);
    }

    static bool isUtf8BomFile(const std::string& path);

    static inline void swap(YJson& A, YJson& B) {
        std::cout << (int)A._type << " - swap - " << B._type << std::endl;
        std::swap(A._type, B._type);
        std::swap(A._value, B._value);
    }

    inline bool isArray() const { return _type == Array; }
    inline bool isObject() const { return _type == Object; }
    inline bool isString() const { return _type == String; }
    inline bool isNumber() const { return _type == Number; }
    inline bool isTrue() const { return _type == True; }
    inline bool isFalse() const { return _type == False; }
    inline bool isNull() const { return _type == Null; }

    inline bool emptyA() { return _value.Array->empty(); }
    inline bool emptyO() { return _value.Object->empty(); }

    inline void clearA() { _value.Array->clear(); }
    inline void clearO() { _value.Object->clear(); }
    inline ArrayIterator beginA() { return _value.Array->begin(); }
    inline ObjectIterator begin0() { return _value.Object->begin(); }
    inline ArrayIterator endA() { return _value.Array->end(); }
    inline ObjectIterator endO() { return _value.Object->end(); }

    friend std::ostream& operator<<(std::ostream& out, const YJson& outJson);

private:
    typedef std::string_view::const_iterator StrIterator;
    YJson::Type _type;
    union JsonValue {
        void* Void;
        double* Double = nullptr;
        std::string* String;
        ObjectType* Object;
        ArrayType* Array;
    } _value;

    void strict_parse(std::string_view str);

    std::string_view::const_iterator parseValue(StrIterator first, StrIterator last);
    void printValue(std::string& pre) const;
    void printValue(std::string& pre, int depth) const;

    StrIterator parseNumber(StrIterator first, StrIterator last);
    void printNumber(std::string& pre) const;

    static StrIterator parseString(std::string& des, StrIterator first, StrIterator last);
    static void printString(std::string& pre, const std::string_view str);

    StrIterator parseArray(StrIterator first, StrIterator last);
    void printArray(std::string& pre) const;
    void printArray(std::string& pre, int depth) const;

    StrIterator parseObject(StrIterator value, StrIterator end);
    void printObject(std::string& pre) const;
    void printObject(std::string& pre, int depth) const;

    inline void clearData() {
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

inline std::ostream& operator<<(std::ostream& out, const YJson& outJson)
{
    return out << outJson.toString(true) << std::endl;
}

#endif
