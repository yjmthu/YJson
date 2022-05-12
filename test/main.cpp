#include "yjson.h"

void write(YJson& json) {
    std::cout << "The json content is below: \n"
        << json;
}

void read(YJson& json) {
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
    array1->append(1);
    array1->append(2.71828);
    array1->append(YJson::True);
    array1->append("犹豫是治愈恐惧的良药");
    array1->append(YJson::Null);
    array1->append(YJson::Array);
    array1->append(YJson::Object);
    std::cout << "Array append: \n" << json
        << "\n----------- Find test ---------\n";
    auto number1 = json.find("number1");
    if (number1) {
        std::cout << "number1 is: " << number1->getValueDouble() << std::endl;
    } else {
        std::cout << "number1 not find\n";
    }
    auto number2 = json.find("number2");
    if (number2) {
        std::cout << "number2 is: "<< number2->getValueDouble() << std::endl;
    } else {
        std::cout << "number2 not find\n";
    }
    auto object1 = json.find("object1");
    if (object1) {
        std::cout << "object1 is: " << object1->size() << std::endl;
    } else {
        std::cout << "object1 not find\n";
    }
    auto object2 = json.find("bad");
    if (object2) {
        std::cout << "object2 is found\n";
    } else {
        std::cout << "object2 is not found\n";
    }
    std::cout << "--------- Swap test ----------\n";
    auto Object2 = json.append(YJson::Object, "object2");
    for (int i=0; i<10; ++i) {
        Object1->append(i, std::to_string(i).c_str());
        Object2->append(i+10, std::to_string(i+10).c_str());
    }
    std::cout << "Form js is: \n" << json << std::endl;
    YJson::swap(*object1, *object2);
    std::cout << "New js is: \n" << json << std::endl;
    std::cout << "----------- Clear test ------------\n";
    Object1->clear();
    Object2->clear();
    std::cout << "New js is: \n" << json << std::endl;
}

int main() {
    YJson json(YJson::Object);
    read(json);
    return 0;
}
