#ifndef _IOT_TLS_H_
#define _IOT_TLS_H_

#include "utils.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include <string>

namespace iot
{
    class TlsConnection
    {
    public:
        class Status
        {
        public:
            Status();
            Status(int _code, const std::string& func);
            Status(const std::string& _error, const std::string& func);
            void Clear();
            int code;
            std::string error;
            std::string failedFunc;

            friend std::ostream& operator<<(std::ostream& out, const TlsConnection::Status& s);
        };

        TlsConnection();
        ~TlsConnection();
        void SetPsk(const std::string& psk, const std::string& pskId);
        void SetPersonalizationData(const std::string& data);
        void SetAddress(const Addr& addr);
        const Addr& GetAddress() const;
        const Status& GetLastError() const;
        int Connect();
        void Close();
        bool IsConnected() const;
        bool IsTimedOut() const;
        std::string GetCiphersuite() const;
        int read(unsigned char* buffer, int len, int timeoutMillisec);
        int write(const unsigned char* buffer, int len, int timeoutMillisec);

    private:
        int OnError(int code, const std::string& func);
        int OnError(const std::string& error, const std::string& func);

        mbedtls_entropy_context m_entropy;
        mbedtls_ctr_drbg_context m_rng;
        mbedtls_ssl_context m_ssl;
        mbedtls_ssl_config m_conf;
        mbedtls_net_context m_socket;

        std::string m_psk;
        std::string m_pskId;
        std::string m_persData;
        Addr m_addr;

        bool m_connected;
        bool m_timedOut;
        Status m_lastError;
    };
}

#endif // _IOT_TLS_H_
