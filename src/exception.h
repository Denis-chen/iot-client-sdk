#ifndef _IOT_EXCEPTION_H_
#define _IOT_EXCEPTION_H_

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
        HttpError(const std::string& url, int httpStatus);
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
