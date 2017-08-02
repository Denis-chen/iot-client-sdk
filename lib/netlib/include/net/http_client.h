#pragma once

#include <net/common.h>
#include <net/tcp_connection.h>
#include <net/tls_connection.h>
#include <string>
#include <map>

namespace net
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

        enum ExecuteStatus
        {
            INVALID = -1,
            SUCCESS,
            INVALID_URL,
            INVALID_METHOD,
            NETWORK_ERROR,
            PARSE_ERROR,
        };

        enum Status
        {
            NON_HTTP_ERROR = -1,
            OK = 200,
        };

        typedef std::map<std::string, std::string> StringMap;

        class Request
        {
        public:
            Request();
            Request(const Url& _url);
            Request(Method _method, const Url& _url);
            void SetMethod(Method _method);
            void SetUrl(const Url& _url);
            void SetHeaders(const StringMap &_headers);
            void SetHeader(const std::string& name, const std::string& value);
            void SetData(const std::string& _data);

            Method method;
            Url url;
            StringMap headers;
            std::string data;
        };

        class Response
        {
        public:
            Response();
            bool IsExecuteOk() const;
            std::string GetHeader(const std::string& name) const;
            void Reset();

            ExecuteStatus executeStatus;
            Status httpStatus;
            StringMap headers;
            std::string data;
        };

        class Network
        {
        public:
            virtual ~Network() {}
            virtual void SetX509CaChain(x509::CaChain& caChain) = 0;
            virtual bool ConnectTo(const Url& url, int timeoutMillisec) = 0;
            virtual void Close() = 0;
            virtual int Read(unsigned char* buffer, int len, int timeoutMillisec) = 0;
            virtual int Write(const unsigned char* buffer, int len, int timeoutMillisec) = 0;
            virtual net::Status GetLastError() = 0;
        };

        class Client
        {
        public:
            Client(Network& network);
            void SetTimeout(int timeoutMillisec);
            const Response& Execute(const Request& request);
            net::Status GetLastError();

        private:
            bool SendRequest(const Request& request, const std::string& method);
            bool Write(const std::string& data);
            bool ReadResponse();

            Network& m_network;
            int m_timeout;
            Response m_response;
            net::Status m_error;
        };

        class NetworkImpl : public Network
        {
        public:
            NetworkImpl();
            virtual ~NetworkImpl() {}
            virtual void SetX509CaChain(x509::CaChain& caChain);
            virtual bool ConnectTo(const Url& url, int timeoutMillisec);
            virtual void Close();
            virtual int Read(unsigned char* buffer, int len, int timeoutMillisec);
            virtual int Write(const unsigned char* buffer, int len, int timeoutMillisec);
            virtual net::Status GetLastError();

        private:
            TcpConnection m_tcpConnection;
            TlsConnection m_tlsConnection;
            Connection *m_connection;
            net::Status m_error;
        };
    }
}
