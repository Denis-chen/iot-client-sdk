#ifndef _IOT_UTILS_H_
#define _IOT_UTILS_H_

#include <json.h>
#include <net/http_client.h>
#include <string>
#include <ostream>
#include <sstream>

namespace iot
{
    template <typename T>
    std::string ToString(const T& value)
    {
        std::ostringstream out;
        out << value;
        return out.str();
    }

    std::string HexEncode(const std::string& str);
    std::string HexDecode(const std::string & data);

    class JsonHttpClient
    {
    public:
        JsonHttpClient();
        json::Object MakePostRequest(const std::string& url, const json::Object& data);
        json::Object MakeGetRequest(const std::string& url);

    private:
        json::Object MakeRequest(net::http::Method method, const std::string & url, const json::Object & data);

        net::http::Client m_client;
        net::http::NetworkImpl m_network;
        net::x509::CaChain m_caChain;
    };

    void Sleep(unsigned long milliSeconds);
}

#endif // _IOT_UTILS_H_
