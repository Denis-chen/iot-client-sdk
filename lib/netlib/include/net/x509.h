#pragma once

#include <net/common.h>
#include <mbedtls/x509_crt.h>
#include <string>

namespace net
{
    namespace x509
    {
        class CaChain
        {
        public:
            CaChain();
            ~CaChain();
            int LoadPemData(const unsigned char * buf, size_t len);
            int LoadPemFile(const std::string& fileName);
            int LoadDefaultData();
            const Status& GetLastError() const;

            mbedtls_x509_crt certificates;

        private:
            Status m_lastError;
        };
    }
}