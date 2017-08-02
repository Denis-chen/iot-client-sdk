#include "utils.h"
#include "exception.h"
#include <stdint.h>

namespace iot
{
    namespace
    {
        const char* hexChars = "0123456789abcdef";
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

    JsonHttpClient::JsonHttpClient() : m_client(m_network)
    {
        m_caChain.LoadDefaultData();
        m_network.SetX509CaChain(m_caChain);
        m_client.SetTimeout(10000);
    }

    json::Object JsonHttpClient::MakeRequest(net::http::Method method, const std::string & url, const json::Object & data)
    {
        net::http::Request request(method, url);
        if (method == net::http::POST || method == net::http::PUT)
        {
            request.SetData(json::ToString(data));
            request.SetHeader("Content-Type", "application/json");
            request.SetHeader("Accept", "application/json");
            request.SetHeader("Charsets", "utf-8");
        }

        const net::http::Response& response = m_client.Execute(request);
        if (!response.IsExecuteOk())
        {
            throw NetworkError(url, ToString(m_client.GetLastError()));
        }

        if (response.httpStatus != net::http::OK)
        {
            throw HttpError(method, url, response);
        }

        json::Object responseJson;
        if (response.data.length() > 0)
        {
            responseJson = json::Parse(response.data);
        }

        return responseJson;
    }

    json::Object JsonHttpClient::MakePostRequest(const std::string& url, const json::Object& data)
    {
        return MakeRequest(net::http::POST, url, data);
    }

    json::Object JsonHttpClient::MakeGetRequest(const std::string& url)
    {
        return MakeRequest(net::http::GET, url, json::Object());
    }
}
