#include <net/tcp_connection.h>
#include "utils.h"
#ifdef _WIN32
#include "winsock2.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

namespace net
{
    TcpConnection::TcpConnection() {}

    TcpConnection::~TcpConnection()
    {
        Close();
    }

    int TcpConnection::Connect()
    {
        if (m_connected)
        {
            return 0;
        }

        mbedtls_net_init(&m_socket);
        int res = mbedtls_net_connect(&m_socket, m_addr.host.c_str(), m_addr.port.c_str(), MBEDTLS_NET_PROTO_TCP);
        if (res)
        {
            mbedtls_net_free(&m_socket);
            return m_lastError.Set(res, mbedtls::strerror(res), "mbedtls_net_connect");
        }

        m_connected = true;
        m_timedOut = false;
        m_lastError.Clear();
        return 0;
    }

    void TcpConnection::Close()
    {
        if (!m_connected)
        {
            return;
        }

        mbedtls_net_free(&m_socket);
        m_connected = false;
    }

    int TcpConnection::Read(unsigned char * buffer, int len)
    {
        return Read(buffer, len, 0);
    }

    int TcpConnection::Read(unsigned char * buffer, int len, int timeoutMillisec)
    {
        if (!m_connected)
        {
            return m_lastError.Set(-1, "Network disconnected", "TcpConnection::Read");
        }

        m_timedOut = false;

        int res = mbedtls_net_recv_timeout(&m_socket, buffer, len, timeoutMillisec);
        if (res <= 0)
        {
            if (res == MBEDTLS_ERR_SSL_TIMEOUT)
            {
                m_timedOut = true;
            }
            else
            {
                m_lastError.Set(res, mbedtls::strerror(res), "mbedtls_net_recv_timeout");
                Close();
            }
        }

        return res;
    }

    int TcpConnection::ReadAll(unsigned char * buffer, int len)
    {
        return ReadAll(buffer, len, 0);
    }

    int TcpConnection::ReadAll(unsigned char * buffer, int len, int timeoutMillisec)
    {
        if (!m_connected)
        {
            return m_lastError.Set(-1, "Network disconnected", "TcpConnection::Read");
        }

        m_timedOut = false;

        int received = 0;
        while (received < len)
        {
            int ret = mbedtls_net_recv_timeout(&m_socket, buffer + received, len - received, timeoutMillisec);
            if (ret <= 0)
            {
                if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
                {
                    m_timedOut = true;
                }
                else
                {
                    m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_net_recv_timeout");
                    Close();
                }

                received = ret;
                break;
            }
            received += ret;
        }

        return received;
    }

    int TcpConnection::Write(const unsigned char * buffer, int len)
    {
        return Write(buffer, len, 0);
    }

    int TcpConnection::Write(const unsigned char * buffer, int len, int timeoutMillisec)
    {
        if (!m_connected)
        {
            return m_lastError.Set(-1, "Network disconnected", "TcpConnection::Write");
        }

        if (len <= 0)
        {
            return 0;
        }

        timeval tv = { timeoutMillisec / 1000, (timeoutMillisec % 1000) * 1000 };
        setsockopt(m_socket.fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&tv), sizeof(struct timeval));

        int res = mbedtls_net_send(&m_socket, buffer, len);
        if (res <= 0)
        {
            m_lastError.Set(res, mbedtls::strerror(res), "mbedtls_ssl_write");
            Close();
            return res;
        }

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        setsockopt(m_socket.fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&tv), sizeof(struct timeval));

        return res;
    }
}
