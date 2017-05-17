#include "exception.h"
#include <fmt/format.h>

namespace iot
{
    Exception::Exception(const std::string & error) : std::runtime_error(error) {}

    NetworkError::NetworkError(const std::string& url, const std::string & error)
        : Exception(fmt::sprintf("HTTP POST request to %s failed: %s", url, error)) {}

    HttpError::HttpError(const std::string & url, int httpStatus)
        : Exception(fmt::sprintf("HTTP POST request to %s returned http status code %d", url, httpStatus)) {}

    CryptoError::CryptoError(const std::string & function, int errorCode)
        : Exception(fmt::sprintf("Crypto error: %s() returned %d", function, errorCode)) {}

    JsonError::JsonError(const std::string & error)
        : Exception(fmt::sprintf("JSON parse error: %s", error)) {}
}