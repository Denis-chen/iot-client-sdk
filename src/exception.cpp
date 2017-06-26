#include "exception.h"
#include <fmt/format.h>

namespace iot
{
    Exception::Exception(const std::string & error) : std::runtime_error(error) {}

    NetworkError::NetworkError(const std::string& url, const std::string & error) :
        Exception(fmt::sprintf("Request to %s failed: %s", url, error)) {}

    HttpError::HttpError(http::Method method, const std::string & url, const http::Response& response) :
        Exception(fmt::sprintf("%s request to %s returned http status code %d", http::MethodToString(method), url, response.status)),
        m_response(response) {}

    const http::Response & HttpError::GetResponse() const
    {
        return m_response;
    }

    CryptoError::CryptoError(const std::string & function, int errorCode) :
        Exception(fmt::sprintf("Crypto error: %s() returned %d", function, errorCode)) {}

    CryptoError::CryptoError(const std::string & error) :
        Exception(fmt::sprintf("Crypto error: %s", error)) {}

    JsonError::JsonError(const std::string & error) :
        Exception(fmt::sprintf("JSON parse error: %s", error)) {}
}