#include "yjson.h"

void construct() {
  // YJson js = { { "hello"sv, "hello"sv }, { "world"sv, "world"sv }, { "!"sv,
  // 123} } ; YJson js = { 1, 2, 3, 4 };
  using namespace std::literals;
  YJson js = YJson::O{{u8"hello"sv, u8"hello"sv},
                      {u8"jk"sv, u8"lm"sv},
                      {u8"峨嵋派", u8"空洞派"}};

  std::cout << js;
}

void ifile() {
  // YJson js("./test/test.json", YJson::Encode::UTF8);
  YJson js("/home/yjmthu/.config/Neobox/Wallhaven.json", YJson::UTF8);
  std::cout << js;
  // js.toFile(u8"./test/temp.json");
}

void ofile() {
  using namespace std::literals;
  YJson js =
      YJson::O{{u8"Hello"sv, 12}, {u8"World", 34}, {u8"!", {1, 2, 3, 4, 5, 6}}};
  js.toFile("./test/text.json");
}

void write() {
  YJson json(YJson::Object), js(YJson ::Array);
  json.append(12, u8"1");
  json.append(2345, u8"2");
  // std::cout << "The json content is below: \n"
  //     << json;
  js.append(u8"123");
  js.append(YJson::Object);
  js.getArray().back().append(u8"456", u8"789");
  // std::cout << "The json content is below: \n"
  //     << js;
  json = js;
  // std::cout << "The json content is below: \n"
  //     << json;
}

void read() {
  YJson json(YJson::Object);
  std::cout << "-------- Append test --------\n";
  json.append(1.234, u8"number1");
  json.append(12345, u8"number2");
  json.append(u8"Hello, World!", u8"str1");
  json.append(u8"加油，奥利给！", u8"str2");
  json.append(YJson::True, u8"bool1");
  json.append(YJson::False, u8"bool2");
  json.append(YJson::Null, u8"null1");
  auto array1 = json.append(YJson::Array, u8"array1");
  auto Object1 = json.append(YJson::Object, u8"object1");
  std::cout << "Object append: \n" << json << std::endl;
  array1->second.append<int>(1);
  array1->second.append<double>(2.71828);
  array1->second.append<YJson::Type>(YJson::True);
  array1->second.append(u8"犹豫是治愈恐惧的良药");
  array1->second.append<YJson::Type>(YJson::Null);
  array1->second.append<YJson::Type>(YJson::Array);
  array1->second.append<YJson::Type>(YJson::Object);
  std::cout << "Array append: \n"
            << array1->second << "\n----------- Find test ---------\n";
  auto number1 = json.find(u8"number1");
  if (number1 != json.endO()) {
    std::cout << "number1 is: " << number1->second.getValueDouble()
              << std::endl;
  } else {
    std::cout << "number1 not find\n";
  }
  auto number2 = json.find(u8"number2");
  if (number2 != json.endO()) {
    std::cout << "number2 is: " << number2->second.getValueDouble()
              << std::endl;
  } else {
    std::cout << "number2 not find\n";
  }
  auto object1 = json.find(u8"object1");
  if (object1 != json.endO()) {
    std::cout << "object1 is: " << object1->second.sizeO() << std::endl;
  } else {
    std::cout << "object1 not find\n";
  }
  auto object2 = json.find(u8"bad");
  if (object2 != json.endO()) {
    std::cout << "object2 is found\n";
  } else {
    std::cout << "object2 is not found\n";
  }
  std::cout << "--------- Swap test ----------\n";
  auto Object2 = json.append(YJson::Object, u8"object2");
  std::cout << "Object2 type " << (int)Object2->second.getType() << std::endl;
  for (int i = 0; i < 10; ++i) {
    Object1->second.append(i, YJson::numberToU8String(i));
    Object2->second.append(i + 10, YJson::numberToU8String(i + 10));
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
  for (const auto& i : json.getObject()) {
    std::cout << s++ << " is: " << i.second;
  }
}

int main() {
  read();
  // write();
  // ifile();
  // ofile();
  // construct();
  return 0;
}
