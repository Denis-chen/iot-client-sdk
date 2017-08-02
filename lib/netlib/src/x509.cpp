#include <net/x509.h>
#include "utils.h"
#include <mbedtls/ssl.h>

namespace net
{
    namespace x509
    {
        CaChain::CaChain()
        {
            mbedtls_x509_crt_init(&certificates);
        }

        CaChain::~CaChain()
        {
            mbedtls_x509_crt_free(&certificates);
        }

        int CaChain::LoadPemData(const unsigned char * buf, size_t len)
        {
            int res = mbedtls_x509_crt_parse(&certificates, buf, len);
            if (res < 0)
            {
                return m_lastError.Set(res, mbedtls::strerror(res), "mbedtls_x509_crt_parse");
            }
            return res;
        }

        int CaChain::LoadPemFile(const std::string& fileName)
        {
            int res = mbedtls_x509_crt_parse_file(&certificates, fileName.c_str());
            if (res < 0)
            {
                return m_lastError.Set(res, mbedtls::strerror(res), "mbedtls_x509_crt_parse_file");
            }
            return res;
        }

        namespace
        {
            static unsigned char CACERT_DATA[] = {
                #include "cacert.inc"
            };
        }

        int CaChain::LoadDefaultData()
        {
            return LoadPemData(CACERT_DATA, sizeof(CACERT_DATA));
        }

        const Status & CaChain::GetLastError() const
        {
            return m_lastError;
        }
    }
}
