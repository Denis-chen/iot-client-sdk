#include "http.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <string.h>

namespace iot
{
    namespace http
    {
        static const char *HEADER_CONTENT_LENGTH = "Content-Length";
        static const time_t TIMEOUT_INFINITE = -1;

        std::string MethodToString(Method method)
        {
            switch (method)
            {
            case GET:
                return "GET";
            case PUT:
                return "PUT";
            case POST:
                return "POST";
            case DEL:
                return "DELETE";
            case HEAD:
                return "HEAD";
            default:
                return "";
            }
        }

        static std::string& TrimLeft(std::string& s, const char *chars)
        {
            size_t found = s.find_first_not_of(chars);

            if (found != std::string::npos)
            {
                s.erase(0, found);
            }
            else
            {
                s.clear();
            }

            return s;

        }

        static std::string& TrimRight(std::string& s, const char *chars)
        {
            size_t found = s.find_last_not_of(chars);

            if (found != std::string::npos)
            {
                s.erase(found + 1);
            }
            else
            {
                s.clear();
            }

            return s;
        }

        Response::Response() : status(NETWORK_ERROR)
        {
        }

        std::string Response::GetHeader(const std::string & name) const
        {
            StringMap::const_iterator i = headers.find(name);
            return i != headers.end() ? i->second : "";
        }

        void Response::Reset()
        {
            status = NETWORK_ERROR;
            data.clear();
            headers.clear();
        }

        Request::Request() : m_method(GET), m_timeout(TIMEOUT_INFINITE)
        {
            Reset();
        }

        Request::Request(const std::string & url) : m_method(GET), m_url(url), m_timeout(TIMEOUT_INFINITE)
        {
            Reset();
        }

        Request::Request(Method method, const std::string & url) : m_method(method), m_url(url), m_timeout(TIMEOUT_INFINITE)
        {
            Reset();
        }

        void Request::Reset()
        {
            m_responseDataStarted = false;
            m_response.Reset();
        }

        bool Request::OnError(const std::string & error, const std::string& details)
        {
            m_lastError = details.empty() ? error : details;
            return false;
        }

        void Request::SetMethod(Method method)
        {
            m_method = method;
        }

        void Request::SetUrl(const std::string & url)
        {
            m_url = url;
        }

        void Request::SetHeaders(const StringMap & headers)
        {
            m_headers = headers;
        }

        void Request::SetHeader(const std::string & name, const std::string & value)
        {
            m_headers[name] = value;
        }

        void Request::SetData(const std::string & data)
        {
            m_data = data;
        }

        void Request::SetTimeout(time_t timeoutSeconds)
        {
            m_timeout = timeoutSeconds;
        }

        const Response & Request::GetResponse()
        {
            return m_response;
        }

        const std::string & Request::GetLastError() const
        {
            return m_lastError;
        }


        struct CurlWriteBuffer
        {
            const char *data;
            int64_t sizeleft;
            Request *request;
        };

        size_t Request::ReadResponse(void* ptr, size_t size, size_t nmemb, void* stream)
        {
            size_t bufferSize = size * nmemb;

            Request* req = (Request*) stream;

            if (req->m_responseDataStarted)
            {
                size_t currSize = req->m_response.data.size();
                req->m_response.data.resize(currSize + bufferSize);
                memcpy((void*)(req->m_response.data.data() + currSize), ptr, bufferSize);
            }

            if (bufferSize == strlen("\r\n") && strncmp((const char*)ptr, "\r\n", bufferSize) == 0)
            {
                req->m_responseDataStarted = true;
            }

            return bufferSize;
        }

        size_t Request::WriteData(void *ptr, size_t size, size_t nmemb, void *stream)
        {
            CurlWriteBuffer *readBuf = (struct CurlWriteBuffer*) stream;
            size_t curlBufSize = size * nmemb;
            size_t retval = 0;

            if (curlBufSize < 1)
            {
                return retval;
            }

            if (readBuf->sizeleft > curlBufSize)
            {
                memcpy(ptr, (const void *) readBuf->data, curlBufSize);
                retval = curlBufSize;
                readBuf->sizeleft -= curlBufSize;
                readBuf->data += curlBufSize;
            }
            else
            {
                memcpy(ptr, (const void *) readBuf->data, readBuf->sizeleft);
                retval = readBuf->sizeleft;
                readBuf->sizeleft = 0;
            }

            readBuf->request->m_responseDataStarted = false;

            return retval;
        }

        size_t Request::HeaderCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
        {
            Request* req = (Request*) userdata;
            const char* line = (const char*) ptr;
            const char* delim = strchr(line, ':');

            if (delim != NULL)
            {
                std::string key(line, delim - line);
                std::string value(delim + 1);
                TrimLeft(value, " ");
                TrimRight(value, "\r\n");
                req->m_response.headers[key] = value;
            }

            return size * nmemb;
        }

        bool Request::Execute()
        {
            Reset();

            CURL* curlSession = curl_easy_init();
            if (curlSession == NULL)
            {
                return OnError("curl_easy_init failed", "");
            }

            curl_easy_setopt(curlSession, CURLOPT_NOSIGNAL, 1);
            curl_easy_setopt(curlSession, CURLOPT_URL, m_url.c_str());
            curl_easy_setopt(curlSession, CURLOPT_WRITEFUNCTION, Request::ReadResponse);
            curl_easy_setopt(curlSession, CURLOPT_WRITEDATA, this);

            CurlWriteBuffer curlBuf;
            curlBuf.data = m_data.data();
            curlBuf.sizeleft = m_data.length();
            curlBuf.request = this;

            if (m_method == POST)
            {
                curl_easy_setopt(curlSession, CURLOPT_POSTFIELDSIZE, m_data.length());
                curl_easy_setopt(curlSession, CURLOPT_POSTFIELDS, m_data.data());
            }
            else if (m_method == PUT)
            {
                curl_easy_setopt(curlSession, CURLOPT_UPLOAD, 1L);
                curl_easy_setopt(curlSession, CURLOPT_READDATA, &curlBuf);
                curl_easy_setopt(curlSession, CURLOPT_READFUNCTION, Request::WriteData);
                curl_easy_setopt(curlSession, CURLOPT_INFILESIZE, m_data.data());
            }
            else if (m_method == DEL)
            {
                curl_easy_setopt(curlSession, CURLOPT_CUSTOMREQUEST, MethodToString(DEL).c_str());
            }
            else if (m_method == HEAD)
            {
                curl_easy_setopt(curlSession, CURLOPT_NOBODY, 1);
            }

            curl_easy_setopt(curlSession, CURLOPT_HEADER, 1);
            curl_easy_setopt(curlSession, CURLOPT_HEADERFUNCTION, Request::HeaderCallback);
            curl_easy_setopt(curlSession, CURLOPT_HEADERDATA, this);

            curl_slist*	headerList = NULL;
            for (StringMap::const_iterator i = m_headers.begin(); i != m_headers.end(); ++i)
            {
                headerList = curl_slist_append(headerList, (i->first + ":" + i->second).c_str());
            }

            curl_easy_setopt(curlSession, CURLOPT_HTTPHEADER, headerList);

            char error[CURL_ERROR_SIZE] = { '\0' };
            curl_easy_setopt(curlSession, CURLOPT_ERRORBUFFER, error);

            //curl_easy_setopt(curlSession, CURLOPT_SSL_VERIFYPEER, false);
            //curl_easy_setopt(curlSession, CURLOPT_SSL_VERIFYHOST, 0);

            if (m_timeout != TIMEOUT_INFINITE)
            {
                curl_easy_setopt(curlSession, CURLOPT_TIMEOUT, m_timeout);
            }

            CURLcode result = curl_easy_perform(curlSession);
            if (result == 0)
            {
                long responseCode;
                curl_easy_getinfo(curlSession, CURLINFO_RESPONSE_CODE, &responseCode);
                m_response.status = (int)responseCode;
            }

            curl_slist_free_all(headerList);
            curl_easy_cleanup(curlSession);

            m_headers.clear();
            m_data.clear();

            if (result != 0)
            {
                return OnError(curl_easy_strerror(result), error);
            }

            return true;
        }
    }
}