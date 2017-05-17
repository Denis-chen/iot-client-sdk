#include "tls.h"
#include "mbedtls/error.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <ostream>

namespace iot
{
    TlsConnection::Status::Status()
    {
        code = 0;
    }

    TlsConnection::Status::Status(int _code, const std::string& func)
    {
        code = _code;
        failedFunc = func;
        if (_code)
        {
            char buf[128];
            mbedtls_strerror(_code, buf, sizeof(buf));
            error = buf;
        }
    }

    std::ostream& operator<<(std::ostream& out, const TlsConnection::Status& s)
    {
        if (s.code == 0)
        {
            out << "OK";
        }
        else
        {
            out << s.failedFunc << " failed with error (" << s.code << "): " << s.error;
        }
        return out;
    }

    void TlsConnection::Status::Clear()
    {
        code = 0;
        failedFunc.clear();
        error.clear();
    }

    TlsConnection::TlsConnection() : m_connected(false), m_lastIoTimedOut(false)
    {
        mbedtls_entropy_init(&m_entropy);
        mbedtls_ctr_drbg_init(&m_rng);

        mbedtls_ssl_config_init(&m_conf);
        mbedtls_ssl_config_defaults(&m_conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
        mbedtls_ssl_conf_rng(&m_conf, mbedtls_ctr_drbg_random, &m_rng);
    }

    TlsConnection::~TlsConnection()
    {
        Close();

        mbedtls_entropy_free(&m_entropy);
        mbedtls_ctr_drbg_free(&m_rng);
        mbedtls_ssl_config_free(&m_conf);
    }

    void TlsConnection::SetPsk(const std::string & psk, const std::string & pskId)
    {
        m_psk = psk;
        m_pskId = pskId;
    }

    void TlsConnection::SetPersonalizationData(const std::string & data)
    {
        m_persData = data;
    }

    void TlsConnection::SetAddress(const Addr & addr)
    {
        m_addr = addr;
    }

    const Addr& TlsConnection::GetAddress() const
    {
        return m_addr;
    }

    int TlsConnection::OnError(int code, const std::string & func)
    {
        m_lastError = Status(code, func);
        return code;
    }

    const TlsConnection::Status & TlsConnection::GetLastError() const
    {
        return m_lastError;
    }

    int TlsConnection::Connect()
    {
        if (m_connected)
        {
            return 0;
        }

        int ret = mbedtls_ctr_drbg_seed(&m_rng, mbedtls_entropy_func, &m_entropy, ToUnsignedChar(m_persData), m_persData.length());
        if (ret)
        {
            return OnError(ret, "mbedtls_ctr_drbg_seed");
        }

        mbedtls_ssl_conf_psk(&m_conf, ToUnsignedChar(m_psk), m_psk.length(), ToUnsignedChar(m_pskId), m_pskId.length());

        mbedtls_ssl_init(&m_ssl);
        ret = mbedtls_ssl_setup(&m_ssl, &m_conf);
        if (ret)
        {
            mbedtls_ssl_free(&m_ssl);
            return OnError(ret, "mbedtls_ssl_setup");
        }

        mbedtls_net_init(&m_socket);
        ret = mbedtls_net_connect(&m_socket, m_addr.host.c_str(), m_addr.port.c_str(), MBEDTLS_NET_PROTO_TCP);
        if (ret)
        {
            mbedtls_net_free(&m_socket);
            mbedtls_ssl_free(&m_ssl);
            return OnError(ret, "mbedtls_net_connect");
        }

        mbedtls_ssl_set_bio(&m_ssl, &m_socket, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

        ret = mbedtls_ssl_handshake(&m_ssl);
        if (ret)
        {
            mbedtls_net_free(&m_socket);
            mbedtls_ssl_free(&m_ssl);
            return OnError(ret, "mbedtls_ssl_handshake");
        }

        m_connected = true;
        m_lastIoTimedOut = false;
        m_lastError.Clear();
        return 0;
    }

    void TlsConnection::Close()
    {
        if (!m_connected)
        {
            return;
        }

        mbedtls_ssl_close_notify(&m_ssl);
        mbedtls_net_free(&m_socket);
        mbedtls_ssl_free(&m_ssl);

        m_connected = false;
    }

    bool TlsConnection::IsConnected() const
    {
        return m_connected;
    }

    bool TlsConnection::IsLastIoTimedOut() const
    {
        return m_lastIoTimedOut;
    }

    std::string TlsConnection::GetCiphersuite() const
    {
        if (!m_connected)
        {
            return "";
        }

        return mbedtls_ssl_get_ciphersuite(&m_ssl);
    }

    int TlsConnection::read(unsigned char * buffer, int len, int timeoutMillisec)
    {
        m_lastIoTimedOut = false;

        mbedtls_ssl_conf_read_timeout(&m_conf, timeoutMillisec);

        int received = 0;
        while (received < len)
        {
            int ret = mbedtls_ssl_read(&m_ssl, buffer + received, len - received);
            if (ret <= 0)
            {
                if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
                {
                    m_lastIoTimedOut = true;
                }
                else
                {
                    OnError(ret, "mbedtls_ssl_read");
                    Close();
                }

                received = ret;
                break;
            }
            received += ret;
        }

        mbedtls_ssl_conf_read_timeout(&m_conf, 0);
        return received;
    }

    int TlsConnection::write(const unsigned char * buffer, int len, int timeoutMillisec)
    {
        if (len <= 0)
        {
            return 0;
        }

        m_lastIoTimedOut = false;

        timeval tv = { timeoutMillisec / 1000, (timeoutMillisec % 1000) * 1000 };
        setsockopt(m_socket.fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));

        int ret = mbedtls_ssl_write(&m_ssl, buffer, len);
        if (ret <= 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                m_lastIoTimedOut = true;
            }
            else
            {
                OnError(ret, "mbedtls_ssl_write");
                Close();
            }
        }

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        setsockopt(m_socket.fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));

        return ret;
    }
}
