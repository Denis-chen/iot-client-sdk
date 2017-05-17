#ifndef _IOT_HTTP_H_
#define _IOT_HTTP_H_

#include <string>
#include <map>
#include <time.h>

namespace iot
{
    namespace http
    {
        enum Method
        {
            UNKNOWN = -1,
            GET,
            PUT,
            POST,
            DEL,
            HEAD,
        };

        std::string MethodToString(Method method);

        enum Status
        {
            NETWORK_ERROR = -1,
            OK = 200,
        };

        typedef std::map<std::string, std::string> StringMap;

        class Response
        {
        public:
            Response();
            std::string GetHeader(const std::string& name) const;
            void Reset();

            int status;
            std::string data;
            StringMap headers;
        };

        class Request
        {
        public:
            Request();
            Request(const std::string& url);
            Request(Method method, const std::string& url);
            void SetMethod(Method method);
            void SetUrl(const std::string& url);
            void SetHeaders(const StringMap &headers);
            void SetHeader(const std::string& name, const std::string& value);
            void SetData(const std::string& data);
            void SetTimeout(time_t timeoutSeconds);
            bool Execute();
            const Response& GetResponse();
            const std::string& GetLastError() const;

        private:
            void Reset();
            bool OnError(const std::string& error, const std::string& details);
            static size_t ReadResponse(void* ptr, size_t size, size_t nmemb, void* stream);
            static size_t WriteData(void *ptr, size_t size, size_t nmemb, void *stream);
            static size_t HeaderCallback(void *ptr, size_t size, size_t nmemb, void *userdata);

            Method m_method;
            std::string m_url;
            StringMap m_headers;
            std::string m_data;
            time_t m_timeout;
            Response m_response;
            bool m_responseDataStarted;
            std::string m_lastError;
        };
    }
}

#endif // _MIOT_HTTP_H_
