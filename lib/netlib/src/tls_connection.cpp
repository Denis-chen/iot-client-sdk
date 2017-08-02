#include "net/tls_connection.h"
#include "utils.h"
#ifdef _WIN32
#include "winsock2.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

namespace net
{
    TlsConnection::TlsConnection() : m_rngSeeded(false)
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
        mbedtls_ssl_conf_psk(&m_conf, ToUnsignedChar(m_psk), m_psk.length(), ToUnsignedChar(m_pskId), m_pskId.length());
    }

    void TlsConnection::SetX509CaChain(x509::CaChain & caChain)
    {
        mbedtls_ssl_conf_ca_chain(&m_conf, &caChain.certificates, NULL);
        mbedtls_ssl_conf_authmode(&m_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    }

    void TlsConnection::SetPersonalizationData(const std::string & data)
    {
        m_persData = data;
    }

    int TlsConnection::Connect()
    {
        if (m_connected)
        {
            return 0;
        }

        int ret = 0;

        if (!m_rngSeeded)
        {
            ret = mbedtls_ctr_drbg_seed(&m_rng, mbedtls_entropy_func, &m_entropy, ToUnsignedChar(m_persData), m_persData.length());
            if (ret)
            {
                return m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_ctr_drbg_seed");
            }
            m_rngSeeded = true;
        }

        mbedtls_ssl_init(&m_ssl);
        ret = mbedtls_ssl_setup(&m_ssl, &m_conf);
        if (ret)
        {
            mbedtls_ssl_free(&m_ssl);
            return m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_ssl_setup");
        }

        if(m_conf.ca_chain)
        {
            ret = mbedtls_ssl_set_hostname(&m_ssl, m_addr.host.c_str());
            if (ret)
            {
                return m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_ssl_set_hostname");
            }
        }

        mbedtls_net_init(&m_socket);
        ret = mbedtls_net_connect(&m_socket, m_addr.host.c_str(), m_addr.port.c_str(), MBEDTLS_NET_PROTO_TCP);
        if (ret)
        {
            mbedtls_net_free(&m_socket);
            mbedtls_ssl_free(&m_ssl);
            return m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_net_connect");
        }

        mbedtls_ssl_set_bio(&m_ssl, &m_socket, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

        ret = mbedtls_ssl_handshake(&m_ssl);
        if (ret)
        {
            mbedtls_net_free(&m_socket);
            mbedtls_ssl_free(&m_ssl);
            return m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_ssl_handshake");
        }

        m_connected = true;
        m_timedOut = false;
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

    std::string TlsConnection::GetCiphersuite() const
    {
        if (!m_connected)
        {
            return "";
        }

        return mbedtls_ssl_get_ciphersuite(&m_ssl);
    }

    int TlsConnection::Read(unsigned char * buffer, int len)
    {
        return Read(buffer, len, 0);
    }

    int TlsConnection::Read(unsigned char * buffer, int len, int timeoutMillisec)
    {
        if (!m_connected)
        {
            return m_lastError.Set(-1, "Network disconnected", "TlsConnection::read");
        }

        m_timedOut = false;

        mbedtls_ssl_conf_read_timeout(&m_conf, timeoutMillisec);

        int ret = mbedtls_ssl_read(&m_ssl, buffer, len);
        if (ret <= 0)
        {
            if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
            {
                m_timedOut = true;
            }
            //else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            //{
            //    ret = 0;
            //}
            else
            {
                m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_ssl_read");
                Close();
            }
        }

        mbedtls_ssl_conf_read_timeout(&m_conf, 0);
        return ret;
    }

    int TlsConnection::ReadAll(unsigned char * buffer, int len)
    {
        return ReadAll(buffer, len, 0);
    }

    int TlsConnection::ReadAll(unsigned char * buffer, int len, int timeoutMillisec)
    {
        if (!m_connected)
        {
            return m_lastError.Set(-1, "Network disconnected", "TlsConnection::read");
        }

        m_timedOut = false;

        mbedtls_ssl_conf_read_timeout(&m_conf, timeoutMillisec);

        int received = 0;
        while (received < len)
        {
            int ret = mbedtls_ssl_read(&m_ssl, buffer + received, len - received);
            if (ret <= 0)
            {
                if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
                {
                    m_timedOut = true;
                }
                else
                {
                    m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_ssl_read");
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

    int TlsConnection::Write(const unsigned char * buffer, int len)
    {
        return Write(buffer, len, 0);
    }

    int TlsConnection::Write(const unsigned char * buffer, int len, int timeoutMillisec)
    {
        if (!m_connected)
        {
            return m_lastError.Set(-1, "Network disconnected", "TlsConnection::write");
        }

        if (len <= 0)
        {
            return 0;
        }

        timeval tv = { timeoutMillisec / 1000, (timeoutMillisec % 1000) * 1000 };
        setsockopt(m_socket.fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&tv), sizeof(struct timeval));

        int ret = mbedtls_ssl_write(&m_ssl, buffer, len);
        if (ret <= 0)
        {
            m_lastError.Set(ret, mbedtls::strerror(ret), "mbedtls_ssl_write");
            Close();
            return ret;
        }

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        setsockopt(m_socket.fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&tv), sizeof(struct timeval));

        return ret;
    }
}
