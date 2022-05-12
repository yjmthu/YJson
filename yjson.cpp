#include <iomanip>
#include <utility>
#include <fstream>
#include <assert.h>

#include "yjson.h"
#include "yencode.h"

typedef unsigned char byte;

std::string_view::const_iterator StrSkip(std::string_view::const_iterator content)
{
    while (*content && static_cast<byte>(*content) <= 32)
        content++;
    return content;
}

constexpr byte utf8bom[] = { 0xEF, 0xBB, 0xBF };
constexpr byte utf16le[] = { 0xFF, 0xFE };

YJson::YJson(const std::string& path, YJson::Encode encode)
{
    std::ifstream file(path, std::ios::in);
    assert(file.is_open());
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    if (size < 6) {
        file.close();
        return ;
    }
    file.seekg(0, std::ios::beg);
    switch (encode) {
    case YJson::UTF8BOM: {
        char* json_string;
        json_string = new char[size - 3];
        file.seekg(3, std::ios::beg);
        file.read(json_string, size - 3);
        file.close();
        strict_parse(std::string_view(json_string, size-3));
        delete[] json_string;
        break;
    } case YJson::UTF8: {
        char* json_string;
        json_string = new char[size];
        file.read(json_string, size);
        file.close();
        strict_parse(std::string_view(json_string, size));
        delete[] json_string;
        break;
    } case YJson::UTF16LEBOM: {
        std::string json_string;
        file.seekg(2, std::ios::beg);
        std::wstring json_wstr(size / sizeof(wchar_t), 0);
        file.read(reinterpret_cast<char*>(&json_wstr[0]), size - 2);
        YEncode::utf16LE_to_utf8<std::string&, std::wstring::const_iterator>(json_string, json_wstr.cbegin());
        file.close();
        std::string_view string_view(json_string);
        strict_parse(string_view);
        break;
    } case YJson::UTF16LE: {
        std::wstring json_wstr;
        json_wstr.resize(size/sizeof(wchar_t) + 1);
        char *ptr = reinterpret_cast<char*>(&json_wstr.front());
        file.read(ptr, size);
        std::string json_string;
        YEncode::utf16LE_to_utf8<std::string&, std::wstring::const_iterator>(json_string, json_wstr.cbegin());
        file.close();
        strict_parse(json_string);
        break;
    } default:
        throw std::runtime_error("Unknown file type.");
    }

}

void YJson::strict_parse(std::string_view str)
{
    auto temp = StrSkip(str.begin());
    switch (*temp) {
        case '{':
            parseObject(temp, str.end());
            break;
        case '[':
            parseArray(temp, str.end());
            break;
        default:
            throw std::errc::illegal_byte_sequence;
            break;
    }
}

std::string YJson::urlEncode() const
{
    assert(_type == YJson::Object);
    std::string param;
    for (const auto& [key, value]: *_value.Object) {
        param += key;
        param.push_back('=');
        switch (value._type) {
        case YJson::Number:
            value.printNumber(param);
            break;
        case YJson::String:
            param.append(YEncode::urlEncode(*value._value.String));
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
        param.push_back('&');
    }
    if (!param.empty()) param.pop_back();
    return param;
}

std::string YJson::urlEncode(const std::string_view url) const
{
    assert(_type == YJson::Object);
    std::string param(url);
    for (const auto& [key, value]: *_value.Object) {
        param += key;
        param.push_back('=');
        switch (value._type) {
        case YJson::Number:
            value.printNumber(param);
            break;
        case YJson::String:
            param.append(YEncode::urlEncode(*value._value.String));
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
        param.push_back('&');
    }
    if (!param.empty()) param.pop_back();
    return param;
}

YJson::~YJson()
{
    switch (_type) {
    case YJson::Array:
        delete _value.Array;
        break;
    case YJson::Object:
        delete _value.Object;
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

bool YJson::joinA(const YJson& js)
{
    if (&js == this)
        return joinA(YJson(*this));
    for (const auto& i: *js._value.Array) {
        _value.Array->emplace_back(i);
    }
    return true;
}

bool YJson::joinO(const YJson& js)
{
    if (&js == this)
        return joinO(YJson(*this));
    for (const auto& [i, j]: *js._value.Object) {
        _value.Object->emplace_back(i, j);
    }
    return true;
}

bool YJson::toFile(const std::string& name, const YJson::Encode& file_encode, bool fmt)
{
    std::string&& buffer = toString(fmt);
    if (buffer.size()) {
        switch (file_encode) {
        case (YJson::UTF16LE): {
            std::wstring data;
            data.push_back(*reinterpret_cast<const wchar_t*>(utf16le));
            YEncode::utf8_to_utf16LE<std::wstring&>(data, buffer);
            data.back() = L'\n';
            std::ofstream outFile(name, std::ios::out | std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(reinterpret_cast<const char*>(data.data()), data.length() * sizeof(wchar_t));
                outFile.close();
            }
            break;
        } case UTF8BOM: {
            std::ofstream outFile(name, std::ios::out | std::ios::binary);
            if (outFile.is_open())
            {
                outFile.write(reinterpret_cast<const char*>(utf8bom), 3);
                outFile << buffer << std::endl;
                outFile.close();
            }
            break;
        } default: {
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

YJson::StrIterator YJson::parseValue(YJson::StrIterator value, YJson::StrIterator end)
{
    if (end <= value)
        return end;
    if (*value=='\"')                   { _type = YJson::String; _value.String = new std::string; return parseString(*_value.String, value, end); }
    if (*value=='-' || (*value>='0' && *value<='9'))    { return parseNumber(value, end); }
    if (*value=='[')                    { return parseArray(value, end); }
    if (*value=='{')                    { return parseObject(value, end); }
    if (std::equal(value, value+4, "null"))     { _type= YJson::Null;  return value+4; }
    if (std::equal(value, value+5, "false"))    { _type= YJson::False; return value+5; }
    if (std::equal(value, value+4, "true"))     { _type= YJson::True;  return value+4; }
    return end;
}

void YJson::printValue(std::string& pre) const
{
    switch (_type) {
    case YJson::Null:
        pre.append("null");
        break;
    case YJson::False:
        pre.append("false");
        break;
    case YJson::True:
        pre.append("true");
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

void YJson::printValue(std::string& pre, int depth) const
{
    switch (_type) {
    case YJson::Null:
        pre.append("null");
        break;
    case YJson::False:
        pre.append("false");
        break;
    case YJson::True:
        pre.append("true");
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

YJson::StrIterator YJson::parseNumber(YJson::StrIterator num, YJson::StrIterator)
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

void YJson::printNumber(std::string& pre) const
{
    const double valuedouble = *_value.Double;
    if (valuedouble == 0) {
        pre.push_back('0');
    } else if (fabs(round(valuedouble) - valuedouble) <= std::numeric_limits<double>::epsilon() && valuedouble <= (double)std::numeric_limits<int>::max() && valuedouble >= (double)std::numeric_limits<int>::min()) {
        char temp[21] = { 0 };
        sprintf(temp, "%.0lf", valuedouble);
        pre.append(temp);
    } else {
        char temp[64] = {0};
        if (fabs(floor(valuedouble)-valuedouble)<=std::numeric_limits<double>::epsilon() && fabs(valuedouble)<1.0e60)
            sprintf(temp,"%.0f",valuedouble);
        else if (fabs(valuedouble)<1.0e-6 || fabs(valuedouble)>1.0e9)
            sprintf(temp,"%e",valuedouble);
        else
            sprintf(temp,"%f",valuedouble);
        pre.append(temp);
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

YJson::StrIterator YJson::parseString(std::string& des, YJson::StrIterator str, YJson::StrIterator end)
{
    YJson::StrIterator ptr = str + 1;
    std::string::iterator ptr2;
    uint32_t uc, uc2;
    size_t len = 0;
    if (*str != '\"') {
        throw std::runtime_error("Invalid String.");
    }
    while (*ptr!='\"' && ptr != end && ++len) if (*ptr++ == '\\') ++ptr;    /* Skip escaped quotes. */
    des.resize(len);
    ptr = str + 1;
    ptr2 = des.begin();
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
    while (*str != '\"' && *str) if (*str++ == '\\') ++str;
    return ++str;
}

void YJson::printString(std::string& pre, const std::string_view str)
{
    const auto size_old = pre.size();
    const char* ptr;
    std::string::iterator ptr2;
    size_t len = 0, flag = 0; unsigned char token;
    
    for (ptr = str.begin(); *ptr; ptr++)
        flag |= ((*ptr > 0 && *ptr < 32) || (*ptr == '\"') || (*ptr == '\\')) ? 1 : 0;
    if (!flag) {
        len = ptr - str.begin();
        pre.resize(size_old + len + 2);
        ptr2 = pre.begin() + size_old;
        *ptr2++ = '\"';
        std::copy(str.begin(), str.end(), ptr2);
        *(ptr2 += len) = '\"';
        return;
    }
    if (str.empty()) {
        pre += "\"\"" ;
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

    pre.resize(size_old + len+2);
    ptr2 = pre.begin() + size_old;
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
}

YJson::StrIterator YJson::parseArray(YJson::StrIterator value, YJson::StrIterator end)
{
    _type = YJson::Array;
    auto& array = *(_value.Array = new ArrayType);
    value = StrSkip(++value);
    if (*value==']') {
        return value + 1;
    }
    array.emplace_back();
    value = StrSkip(array.back().parseValue(StrSkip(value), end));
    if (value >= end) return end;

    while (*value==',') {
        if (*StrSkip(value+1) == ']') {
            return StrSkip(value+1);
        }
        array.emplace_back();
        value = StrSkip(array.back().parseValue(StrSkip(value + 1), end));
        if (value >= end) return end;
    }

    if (*value++ == ']')
        return value;
    throw std::string("未匹配到列表结尾！");
}

void YJson::printArray(std::string& pre) const
{
    if (_value.Array->empty()) {
        pre.append("[]");
        return;
    }
    pre.push_back('[');
    for (const auto& i: *_value.Array) {
        i.printValue(pre);
        pre.push_back(',');
    }
    pre.back() = ']';
}

void YJson::printArray(std::string& pre, int depth) const
{
    if (_value.Array->empty()) {
        pre.append("[]");
        return;
    }
    ++depth;
    pre.push_back('[');
    pre.push_back('\n');
    for (const auto& i: *_value.Array) {
        pre.append(std::string(depth<<2, ' '));
        i.printValue(pre, depth);
        pre.push_back(',');
        pre.push_back('\n');
    }
    auto ptr = pre.end() - 2;
    *ptr = '\n';
    if (--depth) {
        *++ptr = ' ';
        depth *= 4;
        while (--depth) {
            pre.push_back(' ');
        }
        pre.push_back(']');
    } else {
        *++ptr = ']';
    }
}

YJson::StrIterator YJson::parseObject(YJson::StrIterator value, YJson::StrIterator end)
{
    _type = YJson::Object;
    ObjectType& object = *(_value.Object = new ObjectType);
    value = StrSkip(++value);
    if (*value == '}') return value + 1;
    object.emplace_back();
    value = StrSkip(parseString(object.back().first, StrSkip(value), end));
    if (value >= end) return end;
    if (*value != ':') {
        throw std::runtime_error("Invalid Object.");
    }
    value = StrSkip(object.back().second.parseValue(StrSkip(value + 1), end));
    while (*value==',') {
        if (*StrSkip(value+1)=='}') {
            return StrSkip(value+1);
        }
        object.emplace_back();
        value = StrSkip(parseString(object.back().first, StrSkip(value+1), end));
        if (*value != ':') {
            throw std::runtime_error("Invalid Object.");
        }
        value = StrSkip(object.back().second.parseValue(StrSkip(value+1), end));
        if (!(*value)) return end;
    }
    
    if (*value=='}') {
        return value+1;
    }
    throw std::runtime_error("Invalid Object.");
    return end;
}

void YJson::printObject(std::string& pre) const
{
    if (_value.Object->empty()) {
        pre.append("{}");
        return;
    }
    pre.push_back('{');
    for (const auto& [i, j]: *_value.Object) {
        printString(pre, i);
        pre.push_back(':');
        j.printValue(pre);
        pre.push_back(',');
    }
    pre.back() = '}';
}

void YJson::printObject(std::string& pre, int depth) const
{
    if (_value.Object->empty()) {
        pre.append("{}");
        return;
    }
    ++depth;
    pre.push_back('{');
    pre.push_back('\n');
    for (const auto& [i, j]: *_value.Object) {
        pre.append(std::string(depth<<2, ' '));
        printString(pre, i);
        pre.push_back(':');
        pre.push_back(' ');
        j.printValue(pre, depth);
        pre.push_back(',');
        pre.push_back('\n');
    }

    auto ptr = pre.end() - 2;
    *ptr = '\n';
    if (--depth) {
        *++ptr = ' ';
        depth *= 4;
        while (--depth) {
            pre.push_back(' ');
        }
        pre.push_back('}');
    } else {
        *++ptr = '}';
    }
}
