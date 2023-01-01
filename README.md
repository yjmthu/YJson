# YJson

- 使用示例

```cpp
YJson data {
  YJson::O {
    {u8"from", u8"zh-CN"},
    {u8"to",   u8"en-US"},
    {u8"apiKey", u8"xxx"},
    {u8"data", YJson::A {
      u8"2023-01-01", 
      u8"hello",
      0,
      2.5,
      YJson::O {
        {u8"text", YJson::String},
        {u8"id", YJson::Null},
        {u8"pro", true}
      }
    }}
  }
};
data.toFile("usage.json");
```
