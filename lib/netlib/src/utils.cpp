#include "utils.h"
#include "mbedtls/error.h"
#include <algorithm>
#include <cctype>

namespace net
{
    const unsigned char * ToUnsignedChar(const std::string& str)
    {
        return reinterpret_cast<const unsigned char *>(str.c_str());
    }

    namespace mbedtls
    {
        std::string strerror(int code)
        {
            char buf[128];
            mbedtls_strerror(code, buf, sizeof(buf));
            return buf;
        }
    }

    std::string ToLower(const std::string& value)
    {
        std::string res;
        res.resize(value.length());
        std::transform(value.begin(), value.end(), res.begin(), tolower);
        return res;
    }
}
