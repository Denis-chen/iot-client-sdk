#pragma once

#include <net/common.h>
#include <net/x509.h>
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include <string>

namespace net
{
    class TlsConnection : public Connection
    {
    public:
        TlsConnection();
        virtual ~TlsConnection();
        void SetPsk(const std::string& psk, const std::string& pskId);
        void SetX509CaChain(x509::CaChain& caChain);
        void SetPersonalizationData(const std::string& data);
        virtual int Connect();
        virtual void Close();
        virtual int Read(unsigned char* buffer, int len, int timeoutMillisec);
        virtual int Write(const unsigned char* buffer, int len, int timeoutMillisec);
        std::string GetCiphersuite() const;
        int Read(unsigned char* buffer, int len);
        int ReadAll(unsigned char* buffer, int len);
        int ReadAll(unsigned char* buffer, int len, int timeoutMillisec);
        int Write(const unsigned char* buffer, int len);

    private:
        mbedtls_entropy_context m_entropy;
        mbedtls_ctr_drbg_context m_rng;
        mbedtls_ssl_context m_ssl;
        mbedtls_ssl_config m_conf;
        mbedtls_net_context m_socket;
        std::string m_psk;
        std::string m_pskId;
        std::string m_persData;
        bool m_rngSeeded;
    };
}
