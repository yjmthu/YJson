#include "yjson.h"

void construct() {
    // YJson js = { { "hello"sv, "hello"sv }, { "world"sv, "world"sv }, { "!"sv, 123} } ;
    // YJson js = { 1, 2, 3, 4 };
    using namespace std::literals;
    YJson js = { { "hello"s, "hello"sv} };
    std::cout << js;
}

void ifile() {
    // YJson js("./test/test.json", YJson::Encode::UTF8);
    YJson js("/home/yjmthu/.config/Neobox/Wallhaven.json", YJson::UTF8);
    std::cout << js;
}

void ofile() {
    using namespace std::literals;
    YJson js = {{"Hello"s, 12}, {"World", 34}, {"!", {1, 2, 3, 4, 5, 6}}};
    js.toFile("./test/text.json");
}

void write() {
    YJson json(YJson::Object), js(YJson
    ::Array);
    json.append(12, "1");
    json.append(2345, "2");
    std::cout << "The json content is below: \n"
        << json;
    js.append("123");
    js.append(YJson::Object);
    js.getArray().back().append("456", "789");
    std::cout << "The json content is below: \n"
        << js;
    json = js;
    std::cout << "The json content is below: \n"
        << json;
}

void read() {
    YJson json(YJson::Object);
    std::cout << "-------- Append test --------\n";
    json.append(1.234, "number1");
    json.append(12345, "number2");
    json.append("Hello, World!", "str1");
    json.append("加油，奥利给！", "str2");
    json.append(YJson::True, "bool1");
    json.append(YJson::False, "bool2");
    json.append(YJson::Null, "null1");
    auto array1 = json.append(YJson::Array, "array1");
    auto Object1 = json.append(YJson::Object, "object1");
    std::cout << "Object append: \n"
        << json << std::endl;
    array1->second.append<int>(1);
    array1->second.append<double>(2.71828);
    array1->second.append<YJson::Type>(YJson::True);
    array1->second.append("犹豫是治愈恐惧的良药");
    array1->second.append<YJson::Type>(YJson::Null);
    array1->second.append<YJson::Type>(YJson::Array);
    array1->second.append<YJson::Type>(YJson::Object);
    std::cout << "Array append: \n" << array1->second
        << "\n----------- Find test ---------\n";
    auto number1 = json.find("number1");
    if (number1 != json.endO()) {
        std::cout << "number1 is: " << number1->second.getValueDouble() << std::endl;
    } else {
        std::cout << "number1 not find\n";
    }
    auto number2 = json.find("number2");
    if (number2 != json.endO()) {
        std::cout << "number2 is: "<< number2->second.getValueDouble() << std::endl;
    } else {
        std::cout << "number2 not find\n";
    }
    auto object1 = json.find("object1");
    if (object1 != json.endO()) {
        std::cout << "object1 is: " << object1->second.sizeO() << std::endl;
    } else {
        std::cout << "object1 not find\n";
    }
    auto object2 = json.find("bad");
    if (object2 != json.endO()) {
        std::cout << "object2 is found\n";
    } else {
        std::cout << "object2 is not found\n";
    }
    std::cout << "--------- Swap test ----------\n";
    auto Object2 = json.append(YJson::Object, "object2");
    std::cout << "Object2 type " << (int)Object2->second.getType() << std::endl;
    for (int i=0; i<10; ++i) {
        Object1->second.append(i, std::to_string(i).c_str());
        Object2->second.append(i+10, std::to_string(i+10).c_str());
    }
    std::cout << "Form js is: \n" << json << std::endl;
    YJson::swap(Object1->second, Object2->second);
    std::cout << "New js is: \n" << json << std::endl;
    std::cout << "----------- Clear test ------------\n";
    Object1->second.clearO();
    Object2->second.clearO();
    std::cout << "New js is: \n" << json << std::endl;
    std::cout << "----------- Emun test -------------\n";
    int s = 1;
    for (const auto&i: json.getObject()) {
        std::cout << s++ << " is: " << i.second;
    }
}

int main() {
    // read();
    // write();
    ifile();
    // ofile();
    // construct();
    return 0;
}
