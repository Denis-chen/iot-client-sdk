#pragma once

#include <net/common.h>
#include "mbedtls/net_sockets.h"
#include <string>

namespace net
{
    class TcpConnection : public Connection
    {
    public:
        TcpConnection();
        virtual ~TcpConnection();
        virtual int Connect();
        virtual void Close();
        virtual int Read(unsigned char* buffer, int len, int timeoutMillisec);
        virtual int Write(const unsigned char* buffer, int len, int timeoutMillisec);
        int Read(unsigned char* buffer, int len);
        int ReadAll(unsigned char* buffer, int len);
        int ReadAll(unsigned char* buffer, int len, int timeoutMillisec);
        int Write(const unsigned char* buffer, int len);

    private:
        mbedtls_net_context m_socket;
    };
}
