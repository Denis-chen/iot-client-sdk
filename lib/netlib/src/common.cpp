#include <net/common.h>
#include <http_parser.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include "utils.h"
#include <cassert>

namespace net
{
    Status::Code::Code() : m_value(0)
    {
        m_name = "OK";
    }

    Status::Code::Code(int value) : m_value(value)
    {
        m_name = fmt::sprintf("%d", m_value);
    }

    Status::Code::Code(int value, const std::string & name) : m_value(value), m_name(name)
    {
    }

    Status::Code::operator int() const
    {
        return m_value;
    }

    Status::Code::operator std::string() const
    {
        return m_name;
    }

    Status::Status()
    {
    }

    Status::Status(const Code& _code, const std::string& _error, const std::string& func)
    {
        Set(_code, _error, func);
    }

    Status::Status(const Code& _code, const std::string& _error, const std::string& func, const std::string& ctx)
    {
        Set(_code, _error, func, ctx);
    }

    int Status::Set(const Code& _code, const std::string& _error, const std::string& func)
    {
        return Set(_code, _error, func, "");
    }

    int Status::Set(const Code& _code, const std::string& _error, const std::string& func, const std::string& ctx)
    {
        code = _code;
        error = _error;
        failedFunc = func;
        context = ctx;
        return _code;
    }

    void Status::Clear()
    {
        code = Code();
        error.clear();
        failedFunc.clear();
        context.clear();
    }

    std::ostream& operator<<(std::ostream& out, const Status::Code& c)
    {
        out << c.m_name;
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Status& s)
    {
        if (s.code == 0 && s.failedFunc.empty() && s.error.empty())
        {
            out << "OK";
        }
        else
        {
            std::string ctx = s.context.empty() ? fmt::sprintf("code=%s", s.code) : fmt::sprintf("code=%s, %s", s.code, s.context);
            out << s.failedFunc << "() failed (" << ctx << "): " << s.error;
        }
        return out;
    }


    Addr::Addr() {}

    Addr::Addr(const std::string & addr)
    {
        Set(addr);
    }

    Addr::Addr(const std::string & addr, const std::string & defaultPort)
    {
        Set(addr, defaultPort);
    }

    void Addr::Set(const std::string & addr)
    {
        Set(addr, "");
    }

    void Addr::Set(const std::string & addr, const std::string & defaultPort)
    {
        size_t pos = addr.find_first_of(':');
        host = addr.substr(0, pos);
        port = (pos != std::string::npos) ? addr.substr(pos + 1) : defaultPort;
    }

    std::ostream& operator<<(std::ostream& o, const Addr& addr)
    {
        return o << addr.host << ":" << addr.port;
    }


    Url::Url() : valid(false)
    {
    }

    Url::Url(const char * url)
    {
        Set(url);
    }

    Url::Url(const std::string & url)
    {
        Set(url);
    }

    Url::Url(const std::string & _scheme, const std::string & _host, const std::string & _port, const std::string & _path) :
        scheme(_scheme), addr(_host, _port), path(_path), valid(true)
    {
    }

    namespace
    {
        class UrlFields : public http_parser_url
        {
        public:
            UrlFields(const std::string& url)
            {
                http_parser_url_init(this);
                m_dataStart = url.c_str();

                if (http_parser_parse_url(m_dataStart, url.length(), false, this) != 0)
                {
                    m_valid = false;
                    return;
                }
                m_valid = true;
            }

            bool IsValidUrl() const
            {
                return m_valid;
            }

            std::string Get(int fieldIndex, const char *defaultValue = "")
            {
                if (fieldIndex < 0 || fieldIndex >= UF_MAX)
                {
                    assert(false);
                    return "";
                }

                if (field_set & (1 << fieldIndex))
                {
                    const char *data = m_dataStart + field_data[fieldIndex].off;
                    size_t len = field_data[fieldIndex].len;
                    return std::string(data, len);
                }
                else
                {
                    return defaultValue;
                }
            }

        private:
            const char *m_dataStart;
            bool m_valid;
        };
    }

    bool Url::Set(const std::string & url)
    {
        m_originalUrl = url;

        UrlFields fields(url);

        valid = fields.IsValidUrl();
        if (!valid)
        {
            return false;
        }

        scheme = ToLower(fields.Get(UF_SCHEMA, "http"));
        addr.host = ToLower(fields.Get(UF_HOST));

        const char *defaultPort = "";
        if (scheme == "http")
        {
            defaultPort = "80";
        }
        if (scheme == "https")
        {
            defaultPort = "443";
        }

        addr.port = fields.Get(UF_PORT, defaultPort);
        path = fields.Get(UF_PATH);
        std::string query = fields.Get(UF_QUERY);
        if (!query.empty())
        {
            path += "?" + query;
        }

        return true;
    }

    bool Url::IsValid() const
    {
        return valid;
    }

    const std::string & Url::GetScheme() const
    {
        return scheme;
    }

    const std::string & Url::GetHost() const
    {
        return addr.host;
    }

    const std::string & Url::GetPort() const
    {
        return addr.port;
    }

    const std::string & Url::GetPath() const
    {
        return path;
    }

    bool Url::operator==(const Url & other)
    {
        return !(*this != other);
    }

    bool Url::operator!=(const Url & other)
    {
        return scheme != other.scheme || addr.host != other.addr.host || addr.port != other.addr.port || path != other.path;
    }

    std::ostream & operator<<(std::ostream & o, const Url & url)
    {
        if (url.IsValid())
        {
            o << url.scheme << "://" << url.addr << url.path;
        }
        else
        {
            o << "Invalid url: " << url.m_originalUrl;
        }
        return o;
    }


    Connection::Connection() : m_connected(false), m_timedOut(false)
    {
    }

    void Connection::SetAddress(const Addr & addr)
    {
        m_addr = addr;
    }

    const Addr & Connection::GetAddress() const
    {
        return m_addr;
    }

    const Status & Connection::GetLastError() const
    {
        return m_lastError;
    }

    bool Connection::IsConnected() const
    {
        return m_connected;
    }

    bool Connection::IsTimedOut() const
    {
        return m_timedOut;
    }
}
