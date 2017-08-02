#include <net/http_client.h>
#include <net/tcp_connection.h>
#include <net/tls_connection.h>
#include "http_response_parser.h"
#include "utils.h"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace net
{
    namespace http
    {
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

        Request::Request() : method(GET)
        {
        }

        Request::Request(const Url & _url) : method(GET), url(_url)
        {
        }
        
        Request::Request(Method _method, const Url & _url) : method(_method), url(_url)
        {
        }

        void Request::SetMethod(Method _method)
        {
            method = _method;
        }

        void Request::SetUrl(const Url & _url)
        {
            url = _url;
        }

        void Request::SetHeaders(const StringMap & _headers)
        {
            headers = _headers;
        }

        void Request::SetHeader(const std::string & name, const std::string & value)
        {
            headers[name] = value;
        }

        void Request::SetData(const std::string & _data)
        {
            data = _data;
        }


        Response::Response()
        {
            Reset();
        }

        bool Response::IsExecuteOk() const
        {
            return executeStatus == SUCCESS;
        }

        std::string Response::GetHeader(const std::string & name) const
        {
            StringMap::const_iterator i = headers.find(name);
            return i != headers.end() ? i->second : "";
        }

        void Response::Reset()
        {
            executeStatus = INVALID;
            httpStatus = NON_HTTP_ERROR;
            data.clear();
            headers.clear();
        }


        Client::Client(Network & network) : m_network(network), m_timeout(0)
        {
        }

        void Client::SetTimeout(int timeoutMillisec)
        {
            m_timeout = timeoutMillisec;
        }

        const Response & Client::Execute(const Request & request)
        {
            m_response.Reset();

            if (!request.url.IsValid())
            {
                m_error.Set(net::Status::Code(INVALID_URL, "INVALID_URL"), ToString(request.url), "http::Client::Execute");
                m_response.executeStatus = INVALID_URL;
                return m_response;
            }

            std::string method = MethodToString(request.method);
            if (method.empty())
            {
                m_error.Set(net::Status::Code(INVALID_METHOD, "INVALID_METHOD"), fmt::sprintf("Invalid HTTP method: %d", request.method), "http::Client::Execute");
                m_response.executeStatus = INVALID_METHOD;
                return m_response;
            }

            if (!m_network.ConnectTo(request.url, m_timeout))
            {
                m_error = m_network.GetLastError();
                m_response.executeStatus = NETWORK_ERROR;
                return m_response;
            }

            if (!SendRequest(request, method))
            {
                m_network.Close();
                return m_response;
            }

            if (!ReadResponse())
            {
                m_network.Close();
                return m_response;
            }

            m_network.Close();

            m_response.executeStatus = SUCCESS;
            return m_response;
        }

        bool Client::SendRequest(const Request & request, const std::string& method)
        {
            const std::string& path = (!request.url.path.empty()) ? request.url.path : std::string("/");
            if (!Write(fmt::sprintf("%s %s HTTP/1.1\r\n", method, path)))
            {
                return false;
            }

            bool sendRequestBody = false;

            StringMap headers;
            headers["Host"] = request.url.addr.host;
            if (request.method == POST || request.method == PUT)
            {
                sendRequestBody = true;
                headers["Content-Length"] = fmt::sprintf("%lu", request.data.length());
            }
            headers["Connection"] = "close";
            headers.insert(request.headers.begin(), request.headers.end());

            for (StringMap::const_iterator header = headers.begin(); header != headers.end(); ++header)
            {
                if (!Write(fmt::sprintf("%s: %s\r\n", header->first, header->second)))
                {
                    return false;
                }
            }

            if (!Write("\r\n"))
            {
                return false;
            }

            if (sendRequestBody)
            {
                if (!Write(request.data))
                {
                    return false;
                }
            }

            return true;
        }

        bool Client::Write(const std::string & data)
        {
            size_t totalWritten = 0;

            while (totalWritten < data.length())
            {
                const unsigned char *buf = reinterpret_cast<const unsigned char*>(data.c_str() + totalWritten);
                int len = static_cast<int>(data.length() - totalWritten);
                int res = m_network.Write(buf, len, m_timeout);
                if (res <= 0)
                {
                    m_error = m_network.GetLastError();
                    m_response.executeStatus = NETWORK_ERROR;
                    return false;
                }
                totalWritten += res;
            }

            return true;
        }

        bool Client::ReadResponse()
        {
            unsigned char buf[10 * 1024];
            int len = 0;
            ResponseParser parser(m_response);

            while (!parser.IsComplete())
            {
                len = m_network.Read(buf, sizeof(buf), m_timeout);
                if (len <= 0)
                {
                    parser.FeedEOF();
                    break;
                }

                parser.Feed(buf, len);
            }

            if (parser.HasErrors())
            {
                m_error = parser.GetLastError();
                m_response.executeStatus = PARSE_ERROR;
                return false;
            }

            return true;
        }

        net::Status Client::GetLastError()
        {
            return m_error;
        }


        NetworkImpl::NetworkImpl() : m_connection(NULL) {}

        void NetworkImpl::SetX509CaChain(x509::CaChain & caChain)
        {
            m_tlsConnection.SetX509CaChain(caChain);
        }

        bool NetworkImpl::ConnectTo(const Url & url, int timeoutMillisec)
        {
            if (m_connection != NULL)
            {
                Close();
            }

            if (url.scheme == "http")
            {
                m_connection = &m_tcpConnection;
            }
            else if (url.scheme == "https")
            {
                m_connection = &m_tlsConnection;
            }
            else
            {
                m_error.Set(-1, fmt::sprintf("Unsupported protocol for URL: %s", url), "NetworkImpl::ConnectTo");
                return false;
            }

            m_connection->SetAddress(url.addr);
            if (m_connection->Connect() != 0)
            {
                m_error = m_connection->GetLastError();
                m_connection = NULL;
                return false;
            }

            m_error.Clear();
            return true;
        }

        void NetworkImpl::Close()
        {
            if (m_connection != NULL)
            {
                m_connection->Close();
                m_connection = NULL;
            }
        }

        int NetworkImpl::Read(unsigned char * buffer, int len, int timeoutMillisec)
        {
            if (m_connection == NULL)
            {
                return m_error.Set(-1, "Network disconnected", "NetworkImpl::Read");
            }

            int res = m_connection->Read(buffer, len, timeoutMillisec);
            if (!m_connection->IsConnected())
            {
                m_connection = NULL;
            }

            return res;
        }

        int NetworkImpl::Write(const unsigned char * buffer, int len, int timeoutMillisec)
        {
            if (m_connection == NULL)
            {
                return m_error.Set(-1, "Network disconnected", "NetworkImpl::Write");
            }

            int res = m_connection->Write(buffer, len, timeoutMillisec);
            if (!m_connection->IsConnected())
            {
                m_connection = NULL;
            }

            return res;
        }

        net::Status NetworkImpl::GetLastError()
        {
            return m_error;
        }
}
}
