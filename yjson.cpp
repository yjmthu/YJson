#include <fstream>
#include <deque>
#include <limits>
#include <iomanip>
#include <utility>
#include <cmath>
#include <algorithm>
#include <assert.h>

#include "yjson.h"
// #include "ystring.h"
#include "yencode.h"

#ifdef max
#undef max
#endif // max

#ifdef min
#undef min
#endif // min

typedef unsigned char byte;

std::string_view::const_iterator StrSkip(std::string_view::const_iterator content)
{
    while (*content && static_cast<byte>(*content) <= 32)
        content++;
    return content;
}

inline const char* goEnd(const char* begin)
{
    while (*begin) ++begin;
    return begin;
}

const byte YJson::utf8bom[] = { 0xEF, 0xBB, 0xBF };
const byte YJson::utf16le[] = { 0xFF, 0xFE };

YJson* YJson::_nullptr = nullptr;

YJson::YJson(const std::initializer_list<YJson>& lst)
{
    
}

YJson::YJson(const std::initializer_list<const char*>& lst)
{
    YJson* iter, **child = &_value.Child;
    for (const auto& c: lst)
    {
        iter = new YJson();
        iter->_value.String = new std::string(c);
        iter->_type = YJson::String;
        child = &(*child = iter)->_next;
    }
}

YJson::YJson(const std::string& path, YJson::Encode encode)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        throw std::string("文件不可读");
        return;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    if (size < 6)
    {
        file.close();
        return ;
    }
    file.seekg(0, std::ios::beg);
    switch (encode){
    case YJson::UTF8BOM:
    {
        char* json_string;
        json_string = new char[size - 3];
        file.seekg(3, std::ios::beg);
        file.read(json_string, size - 3);
        file.close();
        strict_parse(std::string_view(json_string, size-3));
        delete[] json_string;
        break;
    }
    case YJson::UTF8:
    {
        char* json_string;
        json_string = new char[size];
        file.read(json_string, size);
        file.close();
        strict_parse(std::string_view(json_string, size));
        delete[] json_string;
        break;
    }
    case YJson::UTF16LEBOM:
    {
        std::string json_string;
        file.seekg(2, std::ios::beg);
        std::wstring json_wstr(size / sizeof(wchar_t), 0);
        file.read(reinterpret_cast<char*>(&json_wstr[0]), size - 2);
        YEncode::utf16LE_to_utf8<std::string&, std::wstring::const_iterator>(json_string, json_wstr.cbegin());
        file.close();
        std::string_view string_view(json_string);
        strict_parse(string_view);
        break;
    }
    case YJson::UTF16LE:
    {
        std::wstring json_wstr;
        json_wstr.resize(size/sizeof(wchar_t) + 1);
        char *ptr = reinterpret_cast<char*>(&json_wstr.front());
        file.read(ptr, size);
        std::string json_string;
        YEncode::utf16LE_to_utf8<std::string&, std::wstring::const_iterator>(json_string, json_wstr.cbegin());
        file.close();
        strict_parse(json_string);
        break;
    }
    default:
        throw std::runtime_error("Unknown file type.");
    }

}

void YJson::strict_parse(std::string_view str)
{
    auto temp = StrSkip(str.begin());
    switch (*temp)
    {
        case '{':
            parse_object(temp, str.end());
            break;
        case '[':
            parse_array(temp, str.end());
            break;
        default:
            throw std::errc::illegal_byte_sequence;
            break;
    }
}

// 复制json, 不复制顶层 key
YJson::YJson(const YJson& json)
{
    _type = json._type;
    if (_type == YJson::Array) {
        YJson *child = nullptr, *childx = nullptr;
        if ((child = json._value.Child)) {
            childx = _value.Child = new YJson(*child);
            while ((child = child->_next)) {
                childx->_next = new YJson(*child);
                childx = childx->_next;
            }
        }
    } else if (_type == YJson::Object) {
        YJson *child = nullptr, *childx = nullptr;
        if ((child = json._value.Child)) {
            childx = _value.Child = new YJson(*child);
            childx->_key = new std::string(*child->_key);
            while ((child = child->_next)) {
                childx->_next = new YJson(*child);
                childx = childx->_next;
                childx->_key = new std::string(*child->_key);
            }
        }
    } else if (_type == YJson::String) {
        _value.String = new std::string(*json._value.String);
    } else if (_type == YJson::Number) {
        _value.Double = new double(*json._value.Double);
    }
}

bool YJson::isSameTo(const YJson *other)
{
    if (this == other)
        return true;
    if (this->_type != other->_type)
        return false;
    if (_key) {
        if (!other->_key)
            return false;
        else if (*_key != *other->_key)
            return false;
    } else if (other->_key)
        return false;
    switch (_type) {
    case YJson::Array:
    case YJson::Object:
    {
        YJson* child1 = _value.Child, *child2 = other->_value.Child;
        if (child1) {
            if (!child2)
                return false;
            else {
                if (child1->_next) {
                    if (!child2->_next)
                        return false;
                    else
                        return child1->isSameTo(child2) && child1->_next->isSameTo(child2->_next);
                } else if (child2->_next)
                    return false;
                else
                    return child1->isSameTo(child2);
            }
        } else if (child2)
            return false;
        else
            return true;
    }
    case YJson::Number:
        return *_value.Double == *other->_value.Double;
    case YJson::String:
        return *_value.String == *other->_value.String;
    case YJson::Null:
    case YJson::False:
    case YJson::True:
    default:
        return true;
    }
}

int YJson::size() const
{
    int j = 0; YJson* child = _value.Child;
    if (_value.Child) {
        ++j;
        while ((child = child->_next))
            ++j;
    }
    return j;
}

std::string YJson::urlEncode() const
{
    std::string param;
    if (_type == YJson::Object && _value.Child)
    {
        YJson* ptr = _value.Child;
        do {
            param += *ptr->_key;
            param.push_back('=');
            switch (ptr->_type) {
            case YJson::Number:
                param.append(YEncode::urlEncode(ptr->print_number()));
                break;
            case YJson::String:
                param.append(YEncode::urlEncode(*ptr->_value.String));
                break;
            case YJson::True:
                param.append("true");
                break;
            case YJson::False:
                param.append("false");
                break;
            case YJson::Null:
            default:
                param.append("null");
            }
            if ((ptr = ptr->_next))
                param.push_back('&');
            else
                break;
        } while (true);
    }
    return param;
}

std::string YJson::urlEncode(const std::string_view url) const
{
    std::string param;
    if (_type == YJson::Object && _value.Child)
    {
        YJson* ptr = _value.Child;
        do {
            param += *ptr->_key;
            param.push_back('=');
            switch (ptr->_type) {
            case YJson::Number:
                param.append(YEncode::urlEncode(ptr->print_number()));
                break;
            case YJson::String:
                param.append(YEncode::urlEncode(*ptr->_value.String));
                break;
            case YJson::True:
                param.append("true");
                break;
            case YJson::False:
                param.append("false");
                break;
            case YJson::Null:
            default:
                param.append("null");
            }
            if ((ptr = ptr->_next))
                param.push_back('&');
            else
                break;
        } while (true);
    }
    return std::string(url) + param;
}

YJson::~YJson()
{
    delete _key;
    switch (_type) {
    case YJson::Array:
    case YJson::Object:
        delete _value.Child;
        break;
    case YJson::String:
        delete _value.String;
        break;
    case YJson::Number:
        delete _value.Double;
        break;
    default:
        break;
    }
    delete _next;
}

YJson::Iterator YJson::find(const std::string_view key)
{
    assert(_type == YJson::Object);
    YJson* child = _value.Child;
    if (!child || *child->_key == key)
        return &_value.Child;
    while (child->_next) {
        if (*child->_next->_key == key)
            break;
        child = child->_next;
    }
    return &child->_next;
}

YJson::Iterator YJson::find(int key)
{
    assert(_type == YJson::Array || _type == YJson::Object);
    if (!_value.Child) {
        if (key == 0 || key == -1)
            return &_value.Child;
        throw std::out_of_range("Object or Array has no child.");
    }
    if (key == 0) return &_value.Child;
    else if (key > 0) {
        YJson *ptr = _value.Child;
        while (true) {
            if (--key) {
                if (!(ptr = ptr->_next)) break;
                continue;
            }
            return &ptr->_next;
        }
    } else {
        YJson *ahead = _value.Child, **behand = &_value.Child;
        while (key++) {
            if (!ahead) goto error;
            ahead = ahead->_next;
        }
        while (ahead) {
            ahead = ahead->_next;
            behand = &(*behand)->_next;
        }
        return behand;
    }
error:
    throw std::out_of_range("Index is out of range of Object or Array.");
}

YJson::Iterator YJson::findByVal(const std::string_view value)
{
    assert(_type == YJson::Array || _type == YJson::Object);
    if (!_value.Child || _value.Child->_type == YJson::String && value == *_value.Child->_value.String)
        return &_value.Child;
    YJson *child = _value.Child, *temp = child->_next;
    if (temp) do {
        if (temp->_type == YJson::String && value == *temp->_value.String)
            return &child->_next;
        child = temp;
    } while ((temp = child->_next));
    return &child->_next;
}

YJson::Iterator YJson::findByVal(double value)
{
    assert(_type == YJson::Array || _type == YJson::Object);
    if (!_value.Child || _value.Child->_type == YJson::Number && fabs(value-*_value.Child->_value.Double) <= std::numeric_limits<double>::epsilon())
        return &_value.Child;
    YJson *child = _value.Child, *temp = child->_next;
    if (temp) do {
        if (temp->_type == YJson::Number && fabs(value-*temp->_value.Double) <= std::numeric_limits<double>::epsilon())
            return &child->_next;
        child = temp;
    } while ((temp = child->_next));
    return &child->_next;
}


YJson::Iterator YJson::append(const YJson& js, const std::string_view key)
{
    auto child = append(js._type, key);
    *child = js;
    delete[] child->_key;
    child->_key = new std::string(key);
    return child;
}

YJson::Iterator YJson::append(YJson::Type type)
{
    if (_type == YJson::Object)
        throw std::invalid_argument("Object item key is not provided!");
    YJson **result;
    if (_value.Child) {
        YJson* child = _value.Child;
        while (child->_next) {
            child = child->_next;
        }
        child->_next = new YJson;
        result = &child->_next;
    } else {
        _value.Child = new YJson;
        result = &_value.Child;
    }
    (*result)->_type = type;
    switch (type) {
    case YJson::String:
        break;
    case YJson::Number:
        (*result)->_value.Double = new double(0);
    default:
        break;
    }
    return result;
}

YJson::Iterator YJson::append(YJson::Type type, const std::string_view key)
{
    if (_type != YJson::Object)
        throw std::invalid_argument("Can't give a key to sth is not Object.");
    YJson **result;
    if (_value.Child) {
        YJson* child = _value.Child;
        while (child->_next) {
            child = child->_next;
        }
        child->_next = new YJson;
        result = &child->_next;
    } else {
        _value.Child = new YJson;
        result = &_value.Child;
    }
    (*result)->_type = type;
    (*result)->_key = new std::string(key);
    return result;
}

YJson::Iterator YJson::append(double value)
{
    auto child = append(YJson::Number);
    child->_value.Double = new double(value);
    return child;
}

YJson::Iterator YJson::append(double value, const std::string_view key)
{
    YJson::Iterator child = append(YJson::Number, key);
    child->_value.Double = new double(value);
    return child;
}

YJson::Iterator YJson::append(const std::string_view str)
{
    YJson::Iterator child = append(YJson::String);
    child->_value.String = new std::string(str);
    return child;
}

YJson::Iterator YJson::append(const std::string_view str, const std::string_view key)
{
    YJson::Iterator child = append(YJson::String, key);
    child->_value.String = new std::string(str);
    return child;
}

YJson::Iterator YJson::append(bool val)
{
    return append(YJson::Type(val));
}

YJson::Iterator YJson::append(bool val, const std::string_view key)
{
    return append(YJson::Type(val), key);
}

bool YJson::remove(YJson::Iterator item)
{
    if (!item) return false;
    YJson* temp = *item._data;
    *item._data = item->_next;
    temp->_next = nullptr;
    delete temp;
    return true;
}

bool YJson::isUtf8BomFile(const std::string& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (file.is_open()) {
        unsigned char bom[3];
        if (file.read(reinterpret_cast<char*>(bom), 3)){
            if (std::equal(bom, bom + 3, utf8bom)) {
                file.close();
                return true;
            }
        } else {
            file.close();
        }
    }
    return false;
}

YJson& YJson::operator=(const YJson& s)
{
    return &s == this? *this: (*this = YJson(s));
}

bool YJson::isSameTo(const YJson &other, bool cmpKey)
{
    if (this == &other)
        return true;
    if (this->_type != other._type)
        return false;
    if (cmpKey) {
        if (_key) {
            if (!other._key)
                return false;
            else if (*_key != *other._key)
                return false;
        } else if (other._key)
            return false;
    }
    switch (_type) {
    case YJson::Array:
    case YJson::Object:
    {
        YJson* child1 = _value.Child, *child2 = other._value.Child;
        if (child1) {
            if (!child2)
                return false;
            else {
                if (child1->_next) {
                    if (!child2->_next)
                        return false;
                    else
                        return child1->isSameTo(child2) && child1->_next->isSameTo(child2->_next);
                } else if (child2->_next)
                    return false;
                else
                    return child1->isSameTo(child2);
            }
        } else if (child2)
            return false;
        else
            return true;
    }
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

YJson& YJson::operator=(YJson&& s) noexcept
{
    std::swap(_type, s._type);
    std::swap(_value, s._value);
    return *this;
}

bool YJson::join(const YJson& js)
{
    if (&js == this)
        return join(YJson(*this));
    if (_type != js._type || (_type != YJson::Array && _type != YJson::Object))
        return false;
    YJson *child = _value.Child;
    YJson *childx = js._value.Child;
    if (child) {
        while (child->_next) child = child->_next;
        if (childx) {
            do {
                child->_next = new YJson(*childx);
                child = child->_next;
                child->_key = new std::string(*childx->_key);
            } while ((childx = childx->_next));
        }
    } else if (childx) {
        *this = js;
    }
    return true;
}

YJson YJson::join(const YJson & j1, const YJson & j2)
{
    if ((j1._type != YJson::Array && j1._type != YJson::Object) || j2._type != j1._type) {
        return YJson();
    } else {
        YJson js(j1);
        js.join(j2);
        return YJson(js);
    }
}

std::string YJson::toString(bool fmt)
{
    return fmt? print_value(0):print_value();
}

bool YJson::toFile(const std::string& name, const YJson::Encode& file_encode, bool fmt)
{
    std::string buffer = fmt ? print_value(0): print_value();
    if (buffer.size())
    {
        switch (file_encode) {
        case (YJson::UTF16LE):
        {
            std::wstring data;
            data.push_back(*reinterpret_cast<const wchar_t*>(utf16le));
            YEncode::utf8_to_utf16LE<std::wstring&>(data, buffer);
            data.back() = L'\n';
            std::ofstream outFile(name, std::ios::out | std::ios::binary);
            if (outFile.is_open())
            {
                outFile.write(reinterpret_cast<const char*>(data.data()), data.length() * sizeof(wchar_t));
                outFile.close();
            }
            break;
        }
        case UTF8BOM:
        {
            std::ofstream outFile(name, std::ios::out | std::ios::binary);
            if (outFile.is_open())
            {
                outFile.write(reinterpret_cast<const char*>(utf8bom), 3);
                outFile << buffer << std::endl;
                outFile.close();
            }
            break;
        }
        default:
        {
            std::ofstream outFile(name, std::ios::out | std::ios::binary);
            if (outFile.is_open())
            {
                outFile << buffer << std::endl;
                outFile.close();
            }
            break;
        }
        }
    }
    return false;
}

std::string YJson::joinKeyValue(const std::string_view valuestring, int depth)
{
    std::string buffer((depth <<= 2), ' ');
    if (_key) {
        std::string key = print_string(*_key);
        buffer.resize(depth+key.size()+2+valuestring.size());
        auto ptr = std::copy(key.begin(), key.end(), buffer.begin() + depth);
        *ptr++ = ':';
        *ptr++ = ' ';
        std::copy(valuestring.begin(), valuestring.end(), ptr);
    } else {
        buffer.resize(depth+valuestring.size());
        std::copy(valuestring.begin(), valuestring.end(), buffer.begin()+depth);
    }
    return buffer;
}

std::string YJson::joinKeyValue(const std::string_view valuestring)
{
    if (_key) {
        std::string&& buffer = print_string(*_key) + ":";
        return buffer += valuestring;
    } else {
        return std::string(valuestring);
    }
}

YJson::StrIterator YJson::parse_value(YJson::StrIterator value, YJson::StrIterator end)
{
    if (end <= value)
        return end;
    if (*value=='\"')                   { return parse_string(value, end); }
    if (*value=='-' || (*value>='0' && *value<='9'))    { return parse_number(value, end); }
    if (*value=='[')                    { return parse_array(value, end); }
    if (*value=='{')                    { return parse_object(value, end); }
    if (std::equal(value, value+4, "null"))     { _type= YJson::Null;  return value+4; }
    if (std::equal(value, value+5, "false"))    { _type= YJson::False; return value+5; }
    if (std::equal(value, value+4, "true"))     { _type= YJson::True;  return value+4; }
    return end;
}

std::string YJson::print_value()
{
    switch (_type) {
    case YJson::Null:
        return  joinKeyValue("null", false);
    case YJson::False:
        return joinKeyValue("false", false);
    case YJson::True:
        return joinKeyValue("true", false);
    case YJson::Number:
        return joinKeyValue(print_number(), true);
    case YJson::String:
        return joinKeyValue(print_string(*_value.String), true);;
    case YJson::Array:
        return print_array();
    case YJson::Object:
        return print_object();
    default:
        return nullptr;
    }
}

std::string YJson::print_value(int depth)
{
    switch (_type) {
    case YJson::Null:
        return joinKeyValue("null", depth);
    case YJson::False:
        return joinKeyValue("false", depth);
    case YJson::True:
        return joinKeyValue("true", depth);
    case YJson::Number:
        return joinKeyValue(print_number(), depth);
    case YJson::String:
        return joinKeyValue(print_string(*_value.String), depth);
    case YJson::Array:
        return print_array(depth);
    case YJson::Object:
        return print_object(depth);
    default:
        return nullptr;
    }
}

YJson::StrIterator YJson::parse_number(YJson::StrIterator num, YJson::StrIterator)
{
    _type = YJson::Number;
    _value.Double = new double(0);
    short sign = 1;
    int scale = 0;
    int signsubscale = 1, subscale = 0;
    if (*num=='-') {
        sign = -1;
        ++num;
    }
    if (*num=='0') {
        return ++num;
    }
    if ('1' <= *num && *num <= '9')
        do *_value.Double += (*_value.Double *= 10.0, *num++ - '0');
        while (isdigit(*num));
    if ('.' == *num && isdigit(num[1]))
    {
        ++num;
        do  *_value.Double += (*_value.Double *= 10.0, *num++ - '0'), scale--;
        while (isdigit(*num));
    }
    if ('e' == *num || 'E' == *num)
    {
        if (*++num == '-')
        {
            signsubscale = -1;
            ++num;
        }
        else if (*num == '+')
        {
            ++num;
        }
        while (isdigit(*num))
        {
            subscale *= 10;
            subscale += *num++ - '0';
        }
    }
    *_value.Double *= sign * pow(10, scale + signsubscale * subscale);
    return num;
}

std::string YJson::print_number()
{
    const double valuedouble = *_value.Double;
    if (valuedouble == 0) {
        return std::string(1, '0');
    }
    else if (fabs(round(valuedouble) - valuedouble) <= std::numeric_limits<double>::epsilon() && valuedouble <= (double)std::numeric_limits<int>::max() && valuedouble >= (double)std::numeric_limits<int>::min())
    {
        //qout << "近似" << valuedouble;
        char temp[21] = { 0 };
        sprintf(temp, "%.0lf", valuedouble);
        return temp;
    }
    else
    {
        char temp[64] = {0};
        if (fabs(floor(valuedouble)-valuedouble)<=std::numeric_limits<double>::epsilon() && fabs(valuedouble)<1.0e60)
            sprintf(temp,"%.0f",valuedouble);
        else if (fabs(valuedouble)<1.0e-6 || fabs(valuedouble)>1.0e9)
            sprintf(temp,"%e",valuedouble);
        else
            sprintf(temp,"%f",valuedouble);
        return temp;
    }
}

template<typename T>
inline bool _parse_hex4(T str, uint16_t& h)
{
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

template<typename T>
uint16_t parse_hex4(T str)
{
    uint16_t h = 0;
    if (_parse_hex4(str, h) || _parse_hex4(++str, h = h<<4) || _parse_hex4(++str, h = h<<4) || _parse_hex4(++str, h = h<<4))
        return 0;
    return h;
}

YJson::StrIterator YJson::parse_string(YJson::StrIterator str, YJson::StrIterator end)
{
    YJson::StrIterator ptr = str + 1;
    std::string::iterator ptr2;
    uint32_t uc, uc2;
    size_t len = 0;
    if (*str != '\"') {
        throw std::string("不是字符串");
    }
    while (*ptr!='\"' && ptr != end && ++len) if (*ptr++ == '\\') ++ptr;    /* Skip escaped quotes. */
    _value.String = new std::string(len, 0);
    ptr = str + 1;
    ptr2 = _value.String->begin();
    while (*ptr != '\"' && *ptr)
    {
        if (*ptr != '\\') *ptr2++ = *ptr++;
        else
        {
            ptr++;
            switch (*ptr)
            {
            case 'b': *ptr2++ = '\b';    break;
            case 'f': *ptr2++ = '\f';    break;
            case 'n': *ptr2++ = '\n';    break;
            case 'r': *ptr2++ = '\r';    break;
            case 't': *ptr2++ = '\t';    break;
            case 'u': uc=parse_hex4(ptr+1);ptr+=4;                                                   /* get the unicode char. */

                if ((uc>=YEncode::utf16FirstWcharMark[1] && uc<YEncode::utf16FirstWcharMark[2]) || uc==0)    break;    /* check for invalid.    */

                if (uc>=YEncode::utf16FirstWcharMark[0] && uc<YEncode::utf16FirstWcharMark[1])                         /* UTF16 surrogate pairs.    */
                {
                    if (ptr[1]!='\\' || ptr[2]!='u')    break;                                       /* missing second-half of surrogate.    */
                    uc2=parse_hex4(ptr+3);ptr+=6;
                    if (uc2<YEncode::utf16FirstWcharMark[1] || uc2>=YEncode::utf16FirstWcharMark[2])        break;     /* invalid second-half of surrogate.    */
                    uc=0x10000 + (((uc&0x3FF)<<10) | (uc2&0x3FF));
                }

                len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;

                switch (len) {
                    case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                    case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                    case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                    case 1: *--ptr2 =(uc | YEncode::utf8FirstCharMark[len]);
                }
                ptr2+=len;
                break;
            default:  *ptr2++ = *ptr; break;
            }
            ptr++;
        }
    }
    *ptr2 = 0; ++str;
    _type= YJson::String;
    while (*str != '\"' && *str) if (*str++ == '\\') ++str;
    //qout << "所得字符串：" << _value;
    return ++str;
}

std::string YJson::print_string(const std::string_view str)
{
    std::string buffer;
    //cout << "输出字符串" << str << endl;
    const char* ptr;
    std::string::iterator ptr2;
    size_t len = 0, flag = 0; unsigned char token;
    
    for (ptr = str.begin(); *ptr; ptr++)
        flag |= ((*ptr > 0 && *ptr < 32) || (*ptr == '\"') || (*ptr == '\\')) ? 1 : 0;
    if (!flag)
    {
        len = ptr - str.begin();
        buffer.resize(len+2);
        ptr2 = buffer.begin();
        *ptr2++ = '\"';
        std::copy(str.begin(), str.end(), ptr2);
        *(ptr2 += len) = '\"';
        return buffer;
    }
    if (str.empty())
    {
        buffer = new char[3] { "\"\"" };
        return buffer;
    }
    ptr = str.begin();
    while ((token = (unsigned char)*ptr) && ++len)
    {
        if (strchr("\"\\\b\f\n\r\t", token))
            len++;
        else if (token < 32)
            len += 5;
        ptr++;
    }

    buffer.resize(len+2);
    ptr2 = buffer.begin();
    ptr = str.begin();
    *ptr2++ = '\"';
    while (*ptr)
    {
        if ((unsigned char)*ptr > 31 && *ptr != '\"' && *ptr != '\\') *ptr2++ = *ptr++;
        else
        {
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
    return buffer;
}

YJson::StrIterator YJson::parse_array(YJson::StrIterator value, YJson::StrIterator end)
{
    //qout << "加载列表";
    YJson *child = nullptr;
    _type = YJson::Array;
    value = StrSkip(++value);
    if (*value==']')
    {
        return value+1;    /* empty array. */
    }
    _value.Child = child = new YJson;
    value = StrSkip(child->parse_value(StrSkip(value), end));    /* skip any spacing, get the value. */
    if (value >= end) return end;

    while (*value==',')
    {
        if (*StrSkip(value+1)==']') {
            return StrSkip(value+1);
        }
        child->_next = new YJson;
        child = child->_next;
        value = StrSkip(child->parse_value(StrSkip(value + 1), end));
        if (value >= end) return end;
    }

    if (*value++ == ']')
        return value;
    throw std::string("未匹配到列表结尾！");
}

std::string YJson::print_array()
{
    YJson *child = _value.Child;
    if (!child) {
        return joinKeyValue(std::string_view("[]", 2));
    }
    std::string result(joinKeyValue("["));
    do {
        result.append(child->print_value());
        result.push_back(',');
    } while ((child = child->_next));
    result.back() = ']';
    return result;
}

std::string YJson::print_array(int depth)
{
    YJson *child = _value.Child;
    if (!child)
        return joinKeyValue("[]", depth);
    std::string result(joinKeyValue("[\n", depth));
    do {
        result.append(child->print_value(depth+1));
        result.push_back(',');
        result.push_back('\n');
    } while ((child = child->_next));
    auto ptr = result.end() - 2;
    *ptr = '\n';
    if (depth) {
        *++ptr = ' ';
        depth *= 4;
        while (--depth) {
            result.push_back(' ');
        }
        result.push_back(']');
    } else {
        *++ptr = ']';
    }
    return result;
}

YJson::StrIterator YJson::parse_object(YJson::StrIterator value, YJson::StrIterator end)
{
    YJson *child = nullptr;
    _type = YJson::Object;
    value = StrSkip(++value);
    if (*value == '}') return value + 1;
    _value.Child = child = new YJson;
    value = StrSkip(child->parse_string(StrSkip(value), end));
    if (value >= end) return end;
    child->_key = child->_value.String;
    child->_value.String = nullptr;
    if (*value != ':')
    {
        throw std::string("加载字典时，没有找到冒号");
    }
    value = StrSkip(child->parse_value(StrSkip(value + 1), end));
    while (*value==',')
    {
        if (*StrSkip(value+1)=='}') {
            return StrSkip(value+1);
        }
        child->_next = new YJson;
        child = child->_next;
        value = StrSkip(child->parse_string(StrSkip(value+1), end));
        child->_key = child->_value.String;
        child->_value.String = nullptr;
        if (*value != ':') {
            throw std::string("加载字典时，没有找到冒号");
        }
        value = StrSkip(child->parse_value(StrSkip(value+1), end));    /* skip any spacing, get the value. */
        if (!(*value)) return end;
    }
    
    if (*value=='}') {
        return value+1;
    }
    throw std::string("加载字典时，没有找到结尾大括号");
    return end;
}

std::string YJson::print_object()
{
    YJson *child = _value.Child;
    if (!child) return joinKeyValue("{}");
    std::string buffer(joinKeyValue("{"));
    do {
        buffer.append(child->print_value());
        buffer.push_back(',');
    } while ((child = child->_next));
    buffer.back() = '}';
    return buffer;
}

std::string YJson::print_object(int depth)
{
    YJson *child = _value.Child;
    if (!child) return joinKeyValue("{}", depth);
    std::string result = joinKeyValue("{\n", depth);
    do {
        result.append(child->print_value(depth+1));
        result.push_back(',');
        result.push_back('\n');
    } while ((child = child->_next));

    auto ptr = result.end() - 2;
    *ptr = '\n';
    if (depth) {
        *++ptr = ' ';
        depth *= 4;
        while (--depth) {
            result.push_back(' ');
        }
        result.push_back('}');
    } else {
        *++ptr = '}';
    }
    return result;
}
