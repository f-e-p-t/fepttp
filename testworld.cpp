#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <charconv>

int main(){
    std::string_view str = "342";

    int strint;
    auto result = std::from_chars(str.data(), str.data() + str.size(), strint);

    std::cout << 2 * strint << std::endl;

    return 0;
}