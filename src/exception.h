#ifndef _IOT_EXCEPTION_H_
#define _IOT_EXCEPTION_H_

#include <net/http_client.h>
#include <stdexcept>

namespace iot
{
    class Exception : public std::runtime_error
    {
    public:
        Exception(const std::string & error);
    };

    class NetworkError : public Exception
    {
    public:
        NetworkError(const std::string& url, const std::string & error);
    };

    class HttpError : public Exception
    {
    public:
        HttpError(net::http::Method method, const std::string& url, const net::http::Response& response);
        ~HttpError() throw() {}
        const net::http::Response& GetResponse() const;

    private:
        net::http::Response m_response;
    };

    class CryptoError : public Exception
    {
    public:
        CryptoError(const std::string & function, int errorCode);
        CryptoError(const std::string & error);
    };

    class JsonError : public Exception
    {
    public:
        JsonError(const std::string & error);
    };
}

#endif // _IOT_EXCEPTION_H_
