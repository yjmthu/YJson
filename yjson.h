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
#include <sstream>
#include <fstream>

typedef unsigned char byte;

const byte utf8bom[] = { 0xEF, 0xBB, 0xBF };
const byte utf16le[] = { 0xFF, 0xFE };

class YJson final
{
public:
    inline explicit YJson() { }
    enum Type { False=0, True=1, Null, Number, String, Array, Object };
    enum Encode { UTF8, UTF8BOM };
    typedef std::pair<std::string, YJson> ObjectItemType;
    typedef std::list<ObjectItemType> ObjectType;
    typedef ObjectType::iterator ObjectIterator;
    typedef YJson ArrayItemType;
    typedef std::list<ArrayItemType> ArrayType;
    typedef ArrayType::iterator ArrayIterator;
private:
    typedef std::string_view::const_iterator StrIterator;

    static inline StrIterator StrSkip(StrIterator content) {
        while (*content && static_cast<byte>(*content) <= 32)
            content++;
        return content;
    }
public:
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
    inline YJson(const std::string& str): _type(YJson::String) { _value.String = new std::string(str); }
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

    typedef std::initializer_list<std::pair<const std::string_view, YJson>> O;
    YJson(YJson::O lst): _type(YJson::Type::Object) {
        _value.Object = new ObjectType;
        for (auto& [i, j]: lst) {
            _value.Object->emplace_back(i, std::move(j));
        }
    }

    typedef std::initializer_list<YJson> A;
    YJson(YJson::A lst): _type(YJson::Array) {
        _value.Array = new ArrayType;
        for (auto& i: lst) {
            _value.Array->emplace_back(std::move(i));
        }
    }

    explicit YJson(const std::string& path, Encode encode);
    ~YJson();

    inline YJson::Type getType() const { return _type; }

    inline std::string& getValueString() { return *_value.String; }
    inline const std::string& getValueString() const { return *_value.String; }
    inline int getValueInt() { return *_value.Double; }
    inline double& getValueDouble() { return *_value.Double; }
    inline ObjectType& getObject() { return *_value.Object; }
    inline ArrayType& getArray() { return *_value.Array; }
    std::string urlEncode() const;
    std::string urlEncode(const std::string_view url) const;

    inline int sizeA() const { return _value.Array->size();}
    inline int sizeO() const { return _value.Object->size(); }

    inline std::string toString(bool fmt=false) const {
        std::ostringstream result;
        if (fmt) {
            printValue(result, 0);
        } else {
            printValue(result);
        }
        return result.str();
    }
    inline bool toFile(const std::string& name, bool fmt=true, const Encode& encode=UTF8) {
        std::ofstream result(name, std::ios::out);
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

    inline YJson& operator=(const YJson& other) {
        if (this == &other) return *this;
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

    inline YJson& operator=(YJson&& other) noexcept {
        YJson::swap(*this, other);
        return *this;
    }

    inline YJson& operator=(const std::string_view str) {
        clearData();
        _type = YJson::String;
        _value.String = new std::string(str);
        return *this;
    }

    inline YJson& operator=(const char* str) {
        clearData();
        _type = YJson::String;
        _value.String = new std::string(str);
        return *this;
    }

    inline YJson& operator=(double val) {
        clearData();
        _type = YJson::Number;
        _value.Double = new double(val);
        return *this;
    }
    
    inline YJson& operator=(int val) {
        clearData();
        _type = YJson::Number;
        _value.Double = new double(val);
        return *this;
    }

    inline YJson& operator=(bool val) {
        clearData();
        _type = static_cast<YJson::Type>(val);
        return *this;
    }

    inline YJson& operator=(YJson::Type type) {
        clearData();
        switch (_type = type) {
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

        return *this;
    }

    inline ArrayItemType& operator[](size_t i) {
        return *find(i);
    }

    inline ArrayItemType& operator[](int i) {
        return *find(i);
    }

    inline ObjectItemType& operator[](const char* key) {
        auto itr = find(key);
        if (itr == _value.Object->end()) {
            return *_value.Object->emplace(itr, key, YJson::Null);
        } else {
            return *itr;
        }
    }

    inline ObjectItemType& operator[](const std::string_view key) {
        auto itr = find(key);
        if (itr == _value.Object->end()) {
            return *_value.Object->emplace(itr, key, YJson::Null);
        } else {
            return *itr;
        }

    }

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
    bool operator==(const std::string& str) const { return _type == YJson::String && *_value.String == str; }
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

    // inline void setText(const std::filesystem::path& val) {
    //     clearData();
    //     _type = YJson::String;
    //     _value.String = new std::string(val);
    // }

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
    
    ArrayIterator remove(ArrayIterator item) {
        return _value.Array->erase(item);
    }
    inline ArrayIterator removeA(size_t index) {
        return remove(find(index));
    }
    template<typename _Ty>
    inline ArrayIterator removeByValA(_Ty str) {
        auto iter = findByValA(str);
        return (iter != _value.Array->end())? remove(iter): iter;
    }

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
    
    inline ObjectIterator remove(const std::string_view key) {
        return remove(find(key));
    }
    inline ObjectIterator remove(const char* key) {
        return remove(find(key));
    }
    inline ObjectIterator remove(ObjectIterator item) {
        return _value.Object->erase(item);
    }
    template<typename _Ty>
    inline ObjectIterator removeByValO(_Ty str) {
        auto iter = findByValO(str);
        return (iter != _value.Object->end())? remove(iter): iter;
    }

    static bool isUtf8BomFile(const std::string& path);

    static inline void swap(YJson& A, YJson& B) {
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
    inline ObjectIterator beginO() { return _value.Object->begin(); }
    inline ArrayIterator endA() { return _value.Array->end(); }
    inline ObjectIterator endO() { return _value.Object->end(); }

    void strictParse(std::string_view str);
    inline void parse(std::string_view str) {
        parseValue(StrSkip(str.begin()), str.end());
    }

    friend std::ostream& operator<<(std::ostream& out, const YJson& outJson);

private:
    YJson::Type _type;
    union JsonValue {
        void* Void;
        double* Double = nullptr;
        std::string* String;
        ObjectType* Object;
        ArrayType* Array;
    } _value;

    StrIterator parseValue(StrIterator first, StrIterator last);
    StrIterator parseNumber(StrIterator first, StrIterator last);
    static StrIterator parseString(std::string& des, StrIterator first, StrIterator last);
    StrIterator parseArray(StrIterator first, StrIterator last);
    StrIterator parseObject(StrIterator value, StrIterator end);

    template<typename _Ty>
    void printValue(_Ty& pre) const {
        using namespace std::literals;
        switch (_type) {
        case YJson::Null:
            pre << "null"sv;
            break;
        case YJson::False:
            pre << "false"sv;
            break;
        case YJson::True:
            pre << "true"sv;
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
            throw std::runtime_error("Unknown YJson Type.");
        }
    }
    template<typename _Ty>
    void printValue(_Ty& pre, int depth) const {
        using namespace std::literals;
        switch (_type) {
        case YJson::Null:
            pre << "null"sv;
            break;
        case YJson::False:
            pre << "false"sv;
            break;
        case YJson::True:
            pre << "true"sv;
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
            throw std::runtime_error("Unknown YJson Type.");
        }
    }
    template<typename _Ty>
    void printNumber(_Ty& pre) const {
        const double valuedouble = *_value.Double;
        if (valuedouble == 0) {
            pre << '0';
        } else if (fabs(round(valuedouble) - valuedouble) <= std::numeric_limits<double>::epsilon() && valuedouble <= (double)std::numeric_limits<int>::max() && valuedouble >= (double)std::numeric_limits<int>::min()) {
            char temp[21] = { 0 };
            sprintf(temp, "%.0lf", valuedouble);
            pre << temp;
        } else {
            char temp[64] = {0};
            if (fabs(floor(valuedouble)-valuedouble)<=std::numeric_limits<double>::epsilon() && fabs(valuedouble)<1.0e60)
                sprintf(temp,"%.0f",valuedouble);
            else if (fabs(valuedouble)<1.0e-6 || fabs(valuedouble)>1.0e9)
                sprintf(temp,"%e",valuedouble);
            else
                sprintf(temp,"%f",valuedouble);
            pre << temp;
        }
    }
    template<typename _Ty>
    static void printString(_Ty& pre, const std::string_view str) {
        std::string buffer;
        const char* ptr;
        std::string::iterator ptr2;
        size_t len = 0, flag = 0; unsigned char token;
        
        for (ptr = str.begin(); *ptr; ptr++)
            flag |= ((*ptr > 0 && *ptr < 32) || (*ptr == '\"') || (*ptr == '\\')) ? 1 : 0;
        if (!flag) {
            len = ptr - str.begin();
            buffer.resize(len + 2);
            ptr2 = buffer.begin();
            *ptr2++ = '\"';
            std::copy(str.begin(), str.end(), ptr2);
            *(ptr2 += len) = '\"';
            pre << buffer;
            return;
        }
        if (str.empty()) {
            pre << "\"\"" ;
            return;
        }
        ptr = str.begin();
        while ((token = (unsigned char)*ptr) && ++len) {
            if (strchr("\"\\\b\f\n\r\t", token))
                len++;
            else if (token < 32)
                len += 5;
            ptr++;
        }

        buffer.resize(len + 2);
        ptr2 = buffer.begin();
        ptr = str.begin();
        *ptr2++ = '\"';
        while (*ptr) {
            if ((unsigned char)*ptr > 31 && *ptr != '\"' && *ptr != '\\') *ptr2++ = *ptr++;
            else {
                *ptr2++='\\';
                switch (token = (unsigned char)*ptr++)
                {
                case '\\':    *ptr2++ = '\\';   break;
                case '\"':    *ptr2++ = '\"';   break;
                case '\b':    *ptr2++ = 'b';    break;
                case '\f':    *ptr2++ = 'f';    break;
                case '\n':    *ptr2++ = 'n';    break;
                case '\r':    *ptr2++ = 'r';    break;
                case '\t':    *ptr2++ = 't';    break;
                default: sprintf(ptr2.base(), "u%04x",token); ptr2+=5;    break;
                }
            }
        }
        *ptr2 = '\"';
        pre << buffer;
    }
    template<typename _Ty>
    void printArray(_Ty& pre) const {
        using namespace std::literals;
        if (_value.Array->empty()) {
            pre << "[]"sv;
            return;
        }
        pre << '[';
        auto i = _value.Array->begin(), j = _value.Array->end();
        for (--j; i != j; ++i) {
            i->printValue(pre);
            pre << ',';
        }
        i->printValue(pre);
        pre << ']';
    }
    template<typename _Ty>
    void printArray(_Ty& pre, int depth) const {
        using namespace std::literals;
        if (_value.Array->empty()) {
            pre << "[]";
            return;
        }
        ++depth;
        pre << "[\n"sv;
        auto i = _value.Array->begin(), j = _value.Array->end();
        for (--j; i!=j; ++i) {
            pre << std::string(depth<<2, ' ');
            i->printValue(pre, depth);
            pre << ",\n"sv;
        }
        pre << std::string(depth<<2, ' ');
        i->printValue(pre, depth);
        pre << '\n' << std::string(--depth<<2, ' ') << ']';
    }
    template<typename _Ty>
    void printObject(_Ty& pre) const {
        using namespace std::literals;
        if (_value.Object->empty()) {
            pre << "{}"sv;
            return;
        }
        pre << '{';
        auto i = _value.Object->begin(), j = _value.Object->end();
        for (--j; j!=i; ++i) {
            printString(pre, i->first);
            pre << ':';
            i->second.printValue(pre);
            pre << ',';
        }
        printString(pre, i->first);
        pre << ':';
        i->second.printValue(pre);
        pre << '}';
    }
    template<typename _Ty>
    void printObject(_Ty& pre, int depth) const {
        using namespace std::literals;
        if (_value.Object->empty()) {
            pre << "{}"sv;
            return;
        }
        ++depth;
        pre << "{\n"sv;
        auto i=_value.Object->begin(), j=_value.Object->end();
        for (--j; i!=j; ++i) {
            pre << std::string(depth<<2, ' ');
            printString(pre, i->first);
            pre << ": "sv;
            i->second.printValue(pre, depth);
            pre << ",\n"sv;
        }
        pre << std::string(depth<<2, ' ');
        printString(pre, i->first);
        pre << ": "sv;
        i->second.printValue(pre, depth);
        pre << '\n' << std::string(--depth<<2, ' ') << '}';
    }

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
    outJson.printValue(out, false);
    return out << std::endl;
}

#endif
