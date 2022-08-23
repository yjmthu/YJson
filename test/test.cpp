#include <iostream>

int main()
{
    char pa[] = {0X7F, 0X92, 0X13, 0X01, 0X20, 0X00};
    std::cout << strchr((const char *)pa, 0X81);
    return 0;
}
