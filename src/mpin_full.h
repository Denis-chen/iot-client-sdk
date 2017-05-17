#ifndef _IOT_MPIN_FULL_H_
#define _IOT_MPIN_FULL_H_

#include <iot/client.h>

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
        MPinFull(const std::string& server);
        AuthResult Authenticate(const Identity& id);

    private:
        AuthResult DoAuth(const Identity& id);

        std::string m_server;
    };
}

#endif // _IOT_MPIN_FULL_H_
