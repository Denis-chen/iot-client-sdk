#ifndef _IOT_CRYPTO_H_
#define _IOT_CRYPTO_H_

extern "C"
{
#include <mpin.h>
#include <randapi.h>
}
#include <string>

namespace iot
{
    class Pass1Data
    {
    public:
        Pass1Data(const std::string& _x, const std::string& _sec, const std::string& _u, const std::string& _ut);
        std::string x;
        std::string sec;
        std::string u;
        std::string ut;
    };

    class Pass2Data
    {
    public:
        std::string y;
        std::string v;
        std::string z;
        std::string r;
    };

    class PrecomputeData
    {
    public:
        PrecomputeData();
        PrecomputeData(const std::string& _g1, const std::string& _g2);
        std::string g1;
        std::string g2;
    };

    class AuthData
    {
    public:
        std::string t;
        std::string hm;
        PrecomputeData precomp;
    };

    class Crypto
    {
    public:
        Crypto();
        ~Crypto();
        std::string HashId(const std::string& id);

        Pass1Data Client1(const std::string& mpinId, const std::string& clientSecret);
        std::string Client2(const std::string& x, const std::string& y, const std::string& sec);
        std::string GetG1Multiple(const std::string& hashId, std::string& rOut);
        std::string HashAll(const std::string& hashId, const Pass1Data& pass1, const Pass2Data& pass2, const std::string& t);
        PrecomputeData Precompute(const std::string& token, const std::string& hashId);
        std::string ClientKey(const Pass1Data& pass1, const Pass2Data& pass2, const AuthData& auth);

    private:
        csprng m_rng;
    };
}

#endif // _IOT_CRYPTO_H_
