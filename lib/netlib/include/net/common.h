#pragma once

#include <string>
#include <ostream>

namespace net
{
    class Status
    {
    public:
        class Code
        {
        public:
            Code();
            Code(int value);
            Code(int value, const std::string& name);
            operator int() const;
            operator std::string() const;
            friend std::ostream& operator<<(std::ostream& out, const Code& c);

        private:
            int m_value;
            std::string m_name;
        };

        Status();
        Status(const Code& _code, const std::string& _error, const std::string& func);
        Status(const Code& _code, const std::string& _error, const std::string& func, const std::string& ctx);
        int Set(const Code& _code, const std::string& _error, const std::string& func);
        int Set(const Code& _code, const std::string& _error, const std::string& func, const std::string& ctx);
        void Clear();
        bool IsOK() const;
        friend std::ostream& operator<<(std::ostream& out, const Status& s);

        Code code;
        std::string error;
        std::string failedFunc;
        std::string context;
    };

    class Addr
    {
    public:
        Addr();
        Addr(const std::string& addr);
        Addr(const std::string& addr, const std::string& defaultPort);
        void Set(const std::string& addr);
        void Set(const std::string& addr, const std::string& defaultPort);
        friend std::ostream& operator<<(std::ostream& o, const Addr& addr);

        std::string host;
        std::string port;
    };

    class Url
    {
    public:
        Url();
        Url(const char* url);
        Url(const std::string& url);
        Url(const std::string& _scheme, const std::string& _host, const std::string& _port, const std::string& _path);
        bool Set(const std::string& url);
        bool IsValid() const;
        const std::string& GetScheme() const;
        const std::string& GetHost() const;
        const std::string& GetPort() const;
        const std::string& GetPath() const;
        bool operator==(const Url& other);
        bool operator!=(const Url& other);
        friend std::ostream& operator<<(std::ostream& o, const Url& url);

        std::string scheme;
        Addr addr;
        std::string path;
        bool valid;

    private:
        std::string m_originalUrl;
    };

    class Connection
    {
    public:
        Connection();
        virtual ~Connection() {}
        virtual int Connect() = 0;
        virtual void Close() = 0;
        virtual int Read(unsigned char* buffer, int len, int timeoutMillisec) = 0;
        virtual int Write(const unsigned char* buffer, int len, int timeoutMillisec) = 0;

        void SetAddress(const Addr& addr);
        const Addr& GetAddress() const;
        const Status& GetLastError() const;
        bool IsConnected() const;
        bool IsTimedOut() const;

    protected:
        Addr m_addr;
        bool m_connected;
        bool m_timedOut;
        Status m_lastError;
    };
}
