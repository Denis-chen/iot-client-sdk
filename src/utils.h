#ifndef _IOT_UTILS_H_
#define _IOT_UTILS_H_

#include <json.h>
#include <string>
#include <ostream>

namespace iot
{
    const unsigned char * ToUnsignedChar(const std::string& str);

    std::string HexEncode(const std::string& str);
    std::string HexDecode(const std::string & data);

    class Addr
    {
    public:
        Addr();
        Addr(const std::string& addr, const std::string& defaultPort);
        friend std::ostream& operator<<(std::ostream& o, const Addr& addr);
        std::string host;
        std::string port;
    };

    json::Object MakePostRequest(const std::string& url, const json::Object& data);
    json::Object MakeGetRequest(const std::string& url);
}

#endif // _IOT_UTILS_H_
