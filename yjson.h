#ifndef YJSON_H
#define YJSON_H

#include <cstring>
#include <string>
#include <initializer_list>
#include <system_error>
#include <memory>
#include <iostream>

class YJson final
{
protected:
    inline explicit YJson() { }

public:
    enum Type { False=0, True=1, Null, Number, String, Array, Object };
    enum Encode { AUTO, ANSI, UTF8, UTF8BOM, UTF16LE, UTF16LEBOM, UTF16BE, UTF16BEBOM, OTHER };

    inline explicit YJson(Type type):_type(static_cast<Type>(type)) { }
    YJson(const YJson& js);
    inline explicit YJson(std::ifstream && file) noexcept { loadFile(file); }
    inline explicit YJson(std::ifstream & file) { loadFile(file); }
    inline explicit YJson(const std::string& path, Encode encode) { loadFile(path, encode); }
    inline explicit YJson(const char* path, Encode encode) { loadFile(std::string(path), encode); }
    explicit YJson(const char* str);
    explicit YJson(const std::string& str);
    explicit YJson(const wchar_t*);
    explicit YJson(const std::wstring&);
    YJson(const std::initializer_list<const char*>& lst);
    YJson(const std::initializer_list<YJson>& lst);
    ~YJson();

    inline YJson::Type getType() const { return _type; }
    inline YJson*& getNext() { return _next; }
    inline YJson*& getChild() { return _value.child; }
    // inline YJson* getParent() const { return _parent; }

    inline const char *getKeyString() const {return _key;}
    inline const char *getValueString() const {return _value.value;}
    inline int getValueInt() const { if (_type == YJson::Number) return *reinterpret_cast<double*>(_value.value); else return 0;}
    inline double getValueDouble() const { if (_type == YJson::Number) return *reinterpret_cast<double*>(_value.value); else return 0;}
    std::string urlEncode() const;
    std::string urlEncode(const char* url) const;
    std::string urlEncode(const std::string& url) const;

    int size() const;
    // YJson* getTop() const;

    char* toString(bool fmt=false);
    bool toFile(const std::string name, const YJson::Encode& encode=UTF8, bool fmt=true);
    void loadFile(const std::string& path, YJson::Encode encode=AUTO);

    YJson& operator=(const YJson&);
    YJson& operator=(YJson&&) noexcept;
    bool isSameTo(const YJson& other, bool cmpKey=false);
    inline YJson& operator[](int i) { auto j = find(i); if (!*j) throw std::errc::invalid_seek; else return *j; }
    inline YJson& operator[](const char* key) { auto j = find(key); if (!j) throw std::errc::invalid_seek; else  return *j; }
    inline YJson& operator[](const std::string& key) { auto j = find(key); if (!j) throw std::errc::invalid_seek; else  return *j; }
    inline operator bool() const { const YJson* s = this; return s; }
    inline void setText(const char* val) {
        if (isObject() || isArray()) {
            delete _value.child;
        } else {
            delete [] _value.value;
        }
        _type = YJson::String;
        auto len = strlen(val) + 1;
        _value.value = new char[len];
        std::copy(val, val + len, _value.value);
    }
    inline void setText(const std::string& val) {
        if (isObject() || isArray()) {
            delete _value.child;
        } else {
            delete [] _value.value;
        }
        _type = YJson::String;
        _value.value = new char[val.length() + 1];
        std::copy(val.begin(), val.end(), _value.value);
        _value.value[val.length()] = 0;
    }
    inline void setValue(double val) {
        if (isObject() || isArray()) {
            delete _value.child;
        } else {
            delete [] _value.value;
        }
        _type = YJson::Number;
        memcpy(_value.value = new char[sizeof (double)], &val, sizeof(double));
    }
    inline void setValue(int val) {setValue(static_cast<double>(val));}
    inline void setValue(bool val) {
        if (isObject() || isArray()) {
            delete _value.child;
        } else {
            delete [] _value.value;
        }
        _value.value = nullptr;
        _type = val? True: False;
    }
    inline void setNull() {
        if (isObject() || isArray()) {
            delete _value.child;
        } else {
            delete [] _value.value;
        }
        _value.value = nullptr;
        _type = YJson::Null;
    }
    inline bool setKeyString(const std::string& key) { if (_key) { delete [] _key; _key = new char[key.length() + 1]; std::copy(key.begin(), key.end(), _key); _key[key.length()] = 0; return true; } else return false; }
    inline bool setKeyString(const char* key) { if (!_key) return false; delete [] _key; _key = new char[strlen(key)+1]; std::strcpy(_key, key); return true; }

    bool join(const YJson&);
    static YJson join(const YJson&, const YJson&);

    YJson*& find(int index);
    YJson*& find(const char* key);
    YJson*& find(const std::string& key);
    inline YJson*& findByVal(int value) { return findByVal(static_cast<double>(value)); }
    YJson*& findByVal(double value) ;
    YJson*& findByVal(const char* str) ;

    YJson*& append(const YJson&, const char* key=nullptr);
    YJson*& append(YJson::Type type, const char* key=nullptr);
    inline YJson*& append(int value, const char* key=nullptr) { return append(static_cast<double>(value), key); }
    YJson*& append(double, const char* key=nullptr);
    YJson*& append(const char*, const char* key=nullptr);
    YJson*& append(const std::string&, const char* key=nullptr);
    YJson*& append(bool val, const char* key=nullptr);

    inline bool remove(int index) { return remove(find(index)); }
    inline bool remove(const char* key) { return remove(find(key)); }
    bool remove(YJson*& item);
    inline bool removeByVal(int value) { return remove(findByVal(value)); }
    inline bool removeByVal(double value) { return remove(findByVal(value)); }
    inline bool removeByVal(const std::string & str) { return remove(findByVal(str.c_str())); }
    static bool isUtf8BomFile(const std::string& path);

    inline bool clear() {
        if (isArray() && isObject()) {
            delete _value.child;
            _value.child = nullptr;
            return true;
        } else {
            return false;
        }
    }
    static void swap(YJson&A, YJson&B) {
        std::swap(A._type, B._type);
        std::swap(A._value, B._value);
    }
    inline bool empty(){ return !_value.child; }
    inline bool isArray() const { return _type == Array; }
    inline bool isObject() const { return _type == Object; }
    inline bool isString() const { return _type == String; }
    inline bool isNumber() const { return _type == Number; }
    inline bool isTrue() const { return _type == True; }
    inline bool isFalse() const { return _type == False; }
    inline bool isNull() const { return _type == Null; }

    friend std::ostream& operator<<(std::ostream& out, YJson& outJson);

private:
    static const unsigned char utf8bom[];
    static const unsigned char utf16le[];
    YJson::Type _type = YJson::Null;
    YJson* _next = nullptr;
    char*  _key  = nullptr;
    union JsonValue {char* value = nullptr; YJson* child; } _value;

    template<typename Type>
    void strict_parse(Type value, Type end);

    template<typename Type>
    Type parse_value(Type, Type end);
    char* print_value();
    char* print_value(int depth);

    template<typename Type>
    Type parse_number(Type, Type end);
    char* print_number();

    template<typename Type>
    Type parse_string(Type str, Type end);
    char* print_string(const char* const);

    template<typename Type>
    Type parse_array(Type value, Type end);
    char* print_array();
    char* print_array(int depth);

    template<typename Type>
    Type parse_object(Type value, Type end);
    char* print_object();
    char* print_object(int depth);

    char* joinKeyValue(const char* valuestring, int depth, bool delvalue);
    char* joinKeyValue(const char* valuestring, bool delvalue);

    void loadFile(std::ifstream &);
    bool isSameTo(const YJson* other);
};

inline std::ostream& operator<<(std::ostream& out, YJson& outJson)
{
    std::unique_ptr<char[]> str(outJson.toString(true));
    return out << str.get();
}

#endif
