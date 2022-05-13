#include <iomanip>
#include <utility>
#include <fstream>
#include <assert.h>

#include "yjson.h"
#include "yencode.h"

YJson::YJson(const std::string& path, YJson::Encode encode)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    assert(file.is_open());
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    switch (encode) {
    case YJson::UTF8BOM: {
        char* json_string;
        json_string = new char[size - 3];
        file.seekg(3, std::ios::beg);
        file.read(json_string, size - 3);
        file.close();
        strictParse(std::string_view(json_string, size-3));
        delete[] json_string;
        break;
    } case YJson::UTF8: {
        char* json_string;
        json_string = new char[size];
        file.read(json_string, size);
        file.close();
        strictParse(std::string_view(json_string, size));
        delete[] json_string;
        break;
    } default:
        throw std::runtime_error("Unknown file type.");
    }

}

void YJson::strictParse(std::string_view str)
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
    using namespace std::literals;
    assert(_type == YJson::Object);
    std::ostringstream param;
    for (const auto& [key, value]: *_value.Object) {
        param << key << '=';
        switch (value._type) {
        case YJson::Number:
            value.printNumber(param);
            break;
        case YJson::String:
            param << YEncode::urlEncode(*value._value.String);
            break;
        case YJson::True:
            param << "true"sv;
            break;
        case YJson::False:
            param << "false"sv;
            break;
        case YJson::Null:
        default:
            param << "null"sv;
        }
        param << '&';
    }
    auto&& result = param.str();
    if (!result.empty()) result.pop_back();
    return result;
}

std::string YJson::urlEncode(const std::string_view url) const
{
    using namespace std::literals;
    assert(_type == YJson::Object);
    std::ostringstream param;
    param << url;
    for (const auto& [key, value]: *_value.Object) {
        param << key << '=';
        switch (value._type) {
        case YJson::Number:
            value.printNumber(param);
            break;
        case YJson::String:
            param << YEncode::urlEncode(*value._value.String);
            break;
        case YJson::True:
            param << "true"sv;
            break;
        case YJson::False:
            param << "false"sv;
            break;
        case YJson::Null:
        default:
            param << "null"sv;
        }
        param << '&';
    }
    auto&& result = param.str();
    if (!result.empty()) result.pop_back();
    return result;
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

YJson::StrIterator YJson::parseValue(YJson::StrIterator first, YJson::StrIterator last)
{
    if (last <= first)
        return last;
    if (*first=='\"')                   { _type = YJson::String; _value.String = new std::string; return parseString(*_value.String, first, last); }
    if (*first=='-' || (*first>='0' && *first<='9'))    { return parseNumber(first, last); }
    if (*first=='[')                    { return parseArray(first, last); }
    if (*first=='{')                    { return parseObject(first, last); }
    if (std::equal(first, first+4, "null"))     { _type= YJson::Null;  return first+4; }
    if (std::equal(first, first+5, "false"))    { _type= YJson::False; return first+5; }
    if (std::equal(first, first+4, "true"))     { _type= YJson::True;  return first+4; }
    return last;
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
