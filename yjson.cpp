#include <iomanip>
#include <utility>
#include <fstream>
#include <assert.h>

#include "yjson.h"


inline unsigned char _toHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

inline unsigned char _fromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else throw 0;
    return y;
}

template <typename _CharT>
std::basic_string<_CharT> urlEncode(const std::basic_string_view<_CharT> str)
{
    std::basic_string<_CharT> ret;
    ret.reserve(str.size());
    for (auto i: str)
    {
        if (isalnum(static_cast<unsigned char>(i)) || strchr("-_.~", i))
            ret.push_back(i);
        // else if (i == ' ')
        //     ret.push_back('+');
        else
        {
            ret.push_back('%');
            ret.push_back(_toHex(static_cast<unsigned char>(i) >> 4));
            ret.push_back(_toHex(static_cast<unsigned char>(i) % 16));
        }
    }
    return ret;
}

template <typename _Ty=std::u8string>
std::u8string UrlDecode(const std::u8string& str)
{
    std::u8string ret;
    ret.reserve(str.size());
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] == '+') ret += ' ';
        else if (str[i] == '%')
        {
            if (i + 2 < str.length()) throw 0;
            unsigned char high = _fromHex((unsigned char)str[++i]);
            unsigned char low = _fromHex((unsigned char)str[++i]);
            ret.push_back(high*16 + low);
        }
        else ret.push_back(str[i]);
    }
    return ret;
}

YJson::YJson(const std::filesystem::path& path, YJson::Encode encode)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    assert(file.is_open());
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    switch (encode) {
    case YJson::UTF8BOM: {
        std::u8string json_string;
        json_string.resize(size - 3);
        file.seekg(3, std::ios::beg);
        file.read(reinterpret_cast<char*>(&json_string[0]), size - 3);
        file.close();
        strictParse(json_string);
        break;
    } case YJson::UTF8: {
        std::u8string json_string;
        json_string.resize(size);
        file.read(reinterpret_cast<char*>(&json_string[0]), size);
        file.close();
        strictParse(json_string);
        break;
    } default:
        throw std::runtime_error("Unknown file type.");
    }

}

std::u8string YJson::urlEncode() const
{
    using namespace std::literals;
    assert(_type == YJson::Object);
    std::basic_ostringstream<char8_t> param;
    for (const auto& [key, value]: *_value.Object) {
        param << key << u8'=';
        switch (value._type) {
        case YJson::Number:
            value.printNumber(param);
            break;
        case YJson::String:
            param << ::urlEncode<char8_t>(*value._value.String);
            break;
        case YJson::True:
            param << u8"true"sv;
            break;
        case YJson::False:
            param << u8"false"sv;
            break;
        case YJson::Null:
        default:
            param << u8"null"sv;
        }
        param << '&';
    }
    auto&& result = param.str();
    if (!result.empty()) result.pop_back();
    return result;
}

std::u8string YJson::urlEncode(const std::u8string_view url) const
{
    using namespace std::literals;
    assert(_type == YJson::Object);
    std::basic_ostringstream<char8_t> param;
    param << url;
    for (const auto& [key, value]: *_value.Object) {
        param << key << u8'=';
        switch (value._type) {
        case YJson::Number:
            value.printNumber(param);
            break;
        case YJson::String:
            param << ::urlEncode<char8_t>(*value._value.String);
            break;
        case YJson::True:
            param << u8"true"sv;
            break;
        case YJson::False:
            param << u8"false"sv;
            break;
        case YJson::Null:
        default:
            param << u8"null"sv;
        }
        param << u8'&';
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

bool YJson::isUtf8BomFile(const std::filesystem::path& path)
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
