#include "crypto.h"
#include "exception.h"
#include <fmt/format.h>
#include <string.h>

namespace iot
{
    namespace
    {
        class Octet : public octet
        {
        public:
            Octet(const std::string& input) : m_freeStorage(false)
            {
                this->len = input.length();
                this->max = this->len;
                this->val = const_cast<char *>(input.data());
            }

            Octet(size_t size) : m_freeStorage(true)
            {
                this->len = 0;
                this->max = size;
                this->val = reinterpret_cast<char *>(calloc(size, 1));
            }

            ~Octet()
            {
                if (m_freeStorage)
                {
                    free(this->val);
                }
            }

            operator std::string() const
            {
                return std::string(this->val, this->len);
            }

        private:
            bool m_freeStorage;
        };

        const int G1S = 2 * PFS + 1;
        const int GTS = 12 * PFS;

        void GenerateRandomSeed(char *buf, size_t len)
        {
            if (len == 0)
            {
                return;
            }

            size_t rc = 0;
            FILE *fp = fopen("/dev/urandom", "rb");
            if (fp != NULL)
            {
                rc = fread(buf, 1, len, fp);
                fclose(fp);
            }

            if (rc != len)
            {
                throw CryptoError("GenerateRandomSeed() failed");
            }
        }
    }

    Pass1Data::Pass1Data(const std::string & _x, const std::string & _sec, const std::string & _u, const std::string & _ut)
        : x(_x), sec(_sec), u(_u), ut(_ut) {}

    PrecomputeData::PrecomputeData() {}

    PrecomputeData::PrecomputeData(const std::string & _g1, const std::string & _g2) : g1(_g1), g2(_g2) {}

    Crypto::Crypto() : m_rngCreated(false)
    {
        memset(&m_rng, 0, sizeof(m_rng));
    }

    Crypto::~Crypto()
    {
        if (m_rngCreated)
        {
            KILL_CSPRNG(&m_rng);
        }
    }

    void Crypto::CreateRngOnce()
    {
        if (!m_rngCreated)
        {
            Octet seed(32);
            seed.len = seed.max;
            GenerateRandomSeed(seed.val, seed.len);
            CREATE_CSPRNG(&m_rng, &seed);
            m_rngCreated = true;
        }
    }

    std::string Crypto::HashId(const std::string & id)
    {
        Octet oid(id);
        Octet res(PFS);

        MPIN_HASH_ID(HASH_TYPE_MPIN, &oid, &res);

        return res;
    }

    Pass1Data Crypto::Client1(const std::string & mpinId, const std::string & clientSecret)
    {
        CreateRngOnce();

        Octet oMpinId(mpinId);
        Octet oClientSecret(clientSecret);
        Octet x(PGS);
        Octet sec(G1S);
        Octet u(G1S);
        Octet ut(G1S);

        int res = MPIN_CLIENT_1(HASH_TYPE_MPIN, 0, &oMpinId, &m_rng, &x, 0, &oClientSecret, &sec, &u, &ut, NULL);
        if (res)
        {
            throw CryptoError("MPIN_CLIENT_1", res);
        }

        return Pass1Data(x, sec, u, ut);
    }

    std::string Crypto::Client2(const std::string & x, const std::string & y, const std::string & sec)
    {
        Octet ox(x);
        Octet oy(y);
        std::string secCopy = sec;
        Octet osec(secCopy);

        int res = MPIN_CLIENT_2(&ox, &oy, &osec);
        if (res)
        {
            throw CryptoError("MPIN_CLIENT_2", res);
        }

        return secCopy;
    }

    std::string Crypto::GetG1Multiple(const std::string & hashId, std::string& rOut)
    {
        CreateRngOnce();

        Octet x(PGS);
        Octet g(hashId);
        Octet w(G1S);

        int res = MPIN_GET_G1_MULTIPLE(&m_rng, 1, &x, &g, &w);
        if (res)
        {
            throw CryptoError("MPIN_GET_G1_MULTIPLE", res);
        }

        rOut = x;
        return w;
    }

    std::string Crypto::HashAll(const std::string & hashId, const Pass1Data & pass1, const Pass2Data & pass2, const std::string& t)
    {
        Octet hid(hashId);
        Octet u(pass1.u);
        Octet v(pass2.v);
        Octet y(pass2.y);
        Octet z(pass2.z);
        Octet ot(t);
        Octet hm(PFS);

        MPIN_HASH_ALL(HASH_TYPE_MPIN, &hid, &u, NULL, &y, &v, &z, &ot, &hm);

        return hm;
    }

    PrecomputeData Crypto::Precompute(const std::string & token, const std::string & hashId)
    {
        Octet t(token);
        Octet hid(hashId);
        Octet g1(GTS);
        Octet g2(GTS);

        int res = MPIN_PRECOMPUTE(&t, &hid, NULL, &g1, &g2);
        if (res)
        {
            throw CryptoError("MPIN_PRECOMPUTE", res);
        }

        return PrecomputeData(g1, g2);
    }

    std::string Crypto::ClientKey(const Pass1Data & pass1, const Pass2Data & pass2, const AuthData & auth)
    {
        Octet g1(auth.precomp.g1);
        Octet g2(auth.precomp.g2);
        Octet r(pass2.r);
        Octet x(pass1.x);
        Octet hm(auth.hm);
        Octet t(auth.t);
        Octet key(PAS);

        int res = MPIN_CLIENT_KEY(HASH_TYPE_MPIN, &g1, &g2, 0, &r, &x, &hm, &t, &key);
        if (res)
        {
            throw CryptoError("MPIN_CLIENT_KEY", res);
        }

        return key;
    }
}