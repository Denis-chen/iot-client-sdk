#include "utils.h"
#include "http.h"
#include "exception.h"
#include <stdint.h>

namespace iot
{
    const unsigned char * ToUnsignedChar(const std::string& str)
    {
        return reinterpret_cast<const unsigned char *>(str.c_str());
    }

    namespace
    {
        const char* hexChars = "0123456789abcdef";
    }

    json::Object MakeRequest(const std::string & url, const json::Object & data)
    {
        http::Request request(http::POST, url);
        request.SetTimeout(10);
        request.SetData(json::ToString(data));
        request.SetHeader("Content-Type", "application/json");
        request.SetHeader("Accept", "application/json");
        request.SetHeader("Charsets", "utf-8");
        if (!request.Execute())
        {
            throw NetworkError(url, request.GetLastError());
        }

        http::Response response = request.GetResponse();
        if (response.status != http::OK)
        {
            throw HttpError(url, response.status);
        }

        json::Object responseJson;
        if (response.data.length() > 0)
        {
            responseJson = json::Parse(response.data);
        }

        return responseJson;
    }

    std::string HexEncode(const std::string & data)
    {
        std::string result;
        result.reserve(2 * data.length());
        for (std::string::const_iterator i = data.begin(); i != data.end(); ++i)
        {
            unsigned char c = *i;
            char c1 = hexChars[c >> 4];   // High nibble
            char c2 = hexChars[c & 0x0F]; // Low nibble	
            result += c1;
            result += c2;
        }
        return result;
    }

    std::string HexDecode(const std::string & data)
    {
        size_t len = data.length();
        if (len % 2 != 0)
        {
            return "";
        }

        std::string result;
        result.resize(len / 2);

        for (size_t i = 0; i < len; ++i)
        {
            char c = tolower(data[i]);
            uint8_t nibble = 0;

            switch (c)
            {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                nibble = c - '0';
                break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                nibble = c - 'a' + 0xA;
                break;
            default:
                return "";
            }

            if (i % 2 == 0)
            {
                result[i / 2] = (nibble << 4);
            }
            else
            {
                result[i / 2] |= nibble;
            }
        }

        return result;
    }

    Addr::Addr() {}

    Addr::Addr(const std::string & addr, const std::string & defaultPort)
    {
        size_t pos = addr.find_first_of(':');
        host = addr.substr(0, pos);
        port = (pos != std::string::npos) ? addr.substr(pos + 1) : defaultPort;
    }

    std::ostream& operator<<(std::ostream& o, const Addr& addr)
    {
        return o << addr.host << ":" << addr.port;
    }
}
