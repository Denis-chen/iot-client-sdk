#ifndef _IOT_MPIN_FULL_H_
#define _IOT_MPIN_FULL_H_

#include <iot/client.h>
#include "crypto.h"

namespace iot
{
    class AuthResult
    {
    public:
        std::string clientId;
        std::string sharedSecret;
    };

    class MPinFull
    {
    public:
        MPinFull(Crypto& crypto);
        AuthResult Authenticate(const std::string& server, const Identity& id);

    private:
        AuthResult DoAuth(const std::string& server, const Identity& id);

        Crypto& m_crypto;
    };
}

#endif // _IOT_MPIN_FULL_H_
