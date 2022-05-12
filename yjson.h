#ifndef YJSON_H
#define YJSON_H

#include <cstring>
#include <string>
#include <string_view>
#include <initializer_list>
#include <system_error>
#include <iostream>

class YJson final
{
protected:
    inline explicit YJson() { }
    static YJson* _nullptr;

public:
    enum Type { False=0, True=1, Null, Number, String, Array, Object };
    enum Encode { AUTO, ANSI, UTF8, UTF8BOM, UTF16LE, UTF16LEBOM, UTF16BE, UTF16BEBOM, OTHER };
    class Iterator {
    private:
        friend class YJson;
        YJson** _data;
    public:
        Iterator(YJson** data): _data(data) {}
        Iterator& operator=(const Iterator& other) {
            _data = other._data;
            return *this;
        }
        operator bool() const { return *_data; }
        bool operator==(const Iterator& other) {
            return *_data == *other._data;
        }
        bool operator!=(const Iterator& other) {
            return *_data != *other._data;
        }
        YJson& operator*() {
            return **_data;
        };
        YJson* operator->() const {
            return *_data;
        }
        Iterator& operator++() {
            _data = &(*_data)->_next;
            return *this;
        }
    };

    inline Iterator begin() { return &_value.Child; }
    inline Iterator end() { return &_nullptr; }

    inline explicit YJson(Type type):_type(static_cast<Type>(type)) { }
    YJson(const YJson& js);
    explicit YJson(const std::string& path, Encode encode);
    explicit YJson(const std::string_view str) {strict_parse(str); }
    YJson(const std::initializer_list<const char*>& lst);
    YJson(const std::initializer_list<YJson>& lst);
    ~YJson();

    inline YJson::Type getType() const { return _type; }
    inline Iterator getNext() { return &_next; }
    inline Iterator getChild() { return &_value.Child; }

    inline const std::string& getKeyString() const { return *_key; }
    inline const std::string& getValueString() const { return *_value.String; }
    inline int getValueInt() const { if (_type == YJson::Number) return *_value.Double; else return 0;}
    inline double getValueDouble() const { if (_type == YJson::Number) return *_value.Double; else return 0;}
    std::string urlEncode() const;
    std::string urlEncode(const std::string_view url) const;

    int size() const;

    std::string toString(bool fmt=false);
    bool toFile(const std::string& name, const YJson::Encode& encode=UTF8, bool fmt=true);

    YJson& operator=(const YJson&);
    YJson& operator=(YJson&&) noexcept;
    bool isSameTo(const YJson& other, bool cmpKey=false);
    inline YJson& operator[](int i) { auto j = find(i); if (!*j) throw std::errc::invalid_seek; else return *j; }
    inline YJson& operator[](const char* key) { return operator[](std::string_view(key)); }
    inline YJson& operator[](const std::string_view key) {
        auto j = find(key);
        if (!j)
            throw std::errc::invalid_seek;
        else
            return *j;
    }
    inline operator bool() const { const YJson* s = this; return s; }

    inline void setText(const std::string_view val) {
        clearData();
        _type = YJson::String;
        _value.String = new std::string(val);
    }
    inline void setValue(double val) {
        clearData();
        _type = YJson::Number;
        _value.Double = new double(val);
    }
    inline void setValue(int val) {setValue(static_cast<double>(val));}
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
    inline bool setKeyString(const std::string_view key) {
        if (_key) {
            delete _key;
            _key = new std::string(key);
            return true;
        }
        return false;
    }

    bool join(const YJson&);
    static YJson join(const YJson&, const YJson&);

    Iterator find(int index);
    Iterator find(const std::string_view key);
    inline Iterator find(const char* key) { return find(std::string_view(key)); }
    inline Iterator findByVal(int value) { return findByVal(static_cast<double>(value)); }
    Iterator findByVal(double value) ;
    Iterator findByVal(const std::string_view) ;

    Iterator append(const YJson&);
    Iterator append(YJson::Type type);
    inline Iterator append(int value) { return append(static_cast<double>(value)); }
    Iterator append(double);
    Iterator append(const std::string_view);
    Iterator append(bool val);

    Iterator append(const YJson&, const std::string_view key);
    Iterator append(YJson::Type type, const std::string_view key);
    inline Iterator append(int value, const std::string_view key) { return append(static_cast<double>(value), key); }
    Iterator append(double, const std::string_view key);
    Iterator append(const std::string_view, const std::string_view key);
    Iterator append(bool val, const std::string_view key);

    inline bool remove(int index) { return remove(find(index)); }
    inline bool remove(const std::string_view key) { return remove(find(key)); }
    bool remove(Iterator item);
    inline bool removeByVal(int value) { return remove(findByVal(value)); }
    inline bool removeByVal(double value) { return remove(findByVal(value)); }
    inline bool removeByVal(const std::string_view str) { return remove(findByVal(str)); }
    static bool isUtf8BomFile(const std::string& path);

    inline bool clear() {
        if (isArray() || isObject()) {
            delete _value.Child;
            _value.Child = nullptr;
            return true;
        } else {
            return false;
        }
    }
    static void swap(YJson&A, YJson&B) {
        std::swap(A._type, B._type);
        std::swap(A._value, B._value);
    }
    inline bool empty(){ return !_value.Child; }
    inline bool isArray() const { return _type == Array; }
    inline bool isObject() const { return _type == Object; }
    inline bool isString() const { return _type == String; }
    inline bool isNumber() const { return _type == Number; }
    inline bool isTrue() const { return _type == True; }
    inline bool isFalse() const { return _type == False; }
    inline bool isNull() const { return _type == Null; }

    friend std::ostream& operator<<(std::ostream& out, YJson& outJson);

private:
    typedef std::string_view::const_iterator StrIterator;
    static const unsigned char utf8bom[];
    static const unsigned char utf16le[];
    YJson::Type _type = YJson::Null;
    YJson* _next = nullptr;
    std::string*  _key  = nullptr;
    union JsonValue { void* Void; double* Double = nullptr; std::string* String; YJson* Child; } _value;

    void strict_parse(std::string_view str);

    std::string_view::const_iterator parse_value(StrIterator first, StrIterator last);
    std::string print_value();
    std::string print_value(int depth);

    StrIterator parse_number(StrIterator first, StrIterator last);
    std::string print_number();

    StrIterator parse_string(StrIterator first, StrIterator last);
    std::string print_string(const std::string_view str);

    StrIterator parse_array(StrIterator first, StrIterator last);
    std::string print_array();
    std::string print_array(int depth);

    StrIterator parse_object(StrIterator value, StrIterator end);
    std::string print_object();
    std::string print_object(int depth);

    std::string joinKeyValue(const std::string_view valuestring, int depth);
    std::string joinKeyValue(const std::string_view valuestring);

    bool isSameTo(const YJson* other);

    inline void clearData() {
        if (isObject() || isArray()) {
            delete _value.Child;
        } else if (isString()) {
            delete _value.String;
        } else {
            delete _value.Double;
        }
    }
};

inline std::ostream& operator<<(std::ostream& out, YJson& outJson)
{
    return out << outJson.toString(true) << std::endl;
}

#endif
