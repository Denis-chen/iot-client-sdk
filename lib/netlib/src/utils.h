#pragma once

#include <string>
#include <sstream>

namespace net
{
    const unsigned char * ToUnsignedChar(const std::string& str);

    namespace mbedtls
    {
        std::string strerror(int code);
    }

    template <typename T>
    std::string ToString(const T& value)
    {
        std::ostringstream out;
        out << value;
        return out.str();
    }

    std::string ToLower(const std::string& value);
}
