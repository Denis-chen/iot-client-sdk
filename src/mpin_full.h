#ifndef _IOT_MPIN_FULL_H_
#define _IOT_MPIN_FULL_H_

#include <iot/client.h>
#include "crypto.h"

namespace json
{
    class Object;
}

namespace iot
{
    class HttpError;

    class AuthResult
    {
    public:
        AuthResult();

        std::string clientId;
        std::string sharedSecret;
        bool identityChanged;
        Identity newIdentity;
    };

    class MPinFull
    {
    public:
        MPinFull(Crypto& crypto);
        AuthResult Authenticate(const std::string& server, const Identity& id);

    private:
        AuthResult DoAuth(const std::string& server, const Identity& id);
        Identity RenewExpiredIdentity(const json::Object& renewSecret, const Identity & expiredId);

        Crypto& m_crypto;
    };
}

#endif // _IOT_MPIN_FULL_H_
