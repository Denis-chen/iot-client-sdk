#include "exception.h"
#include <fmt/format.h>
#include <string.h>
#include "crypto.h"

namespace iot
{
    namespace
    {
        class Octet : public octet
        {
        public:
            Octet(const std::string& input) : m_freeStorage(false)
            {
                this->len = static_cast<int>(input.length());
                this->max = this->len;
                this->val = const_cast<char *>(input.data());
            }

            Octet(size_t size) : m_freeStorage(true)
            {
                this->len = 0;
                this->max = static_cast<int>(size);
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
            Octet(const Octet& other);
            Octet& operator=(const Octet& other);

            bool m_freeStorage;
        };

        const int G1S = 2 * PFS + 1;
        const int G2S = 4 * PFS;
        const int GTS = 12 * PFS;

        void GenerateRandomSeed(char *buf, size_t len)
        {
            if (len == 0)
            {
                return;
            }

#ifndef _WIN32
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
#else
            srand(static_cast<unsigned int>(time(NULL)));
            for (size_t i = 0; i < len; ++i)
            {
                int r = (rand() * 256) / RAND_MAX;
                buf[i] = r;
            }
#endif
        }

        void FillWithRandomData(Octet& dest, csprng& rng)
        {
            dest.len = dest.max;
            for (int i = 0; i < dest.len; ++i)
            {
                dest.val[i] = RAND_byte(&rng);
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

    std::string Crypto::SharedKey(const Pass1Data & pass1, const Pass2Data & pass2, const AuthData & auth)
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

    std::string Crypto::RecombineClientSecret(const std::string & share1, const std::string & share2)
    {
        Octet cs1(share1);
        Octet cs2(share2);
        Octet clientSecret(G1S);

        int res = MPIN_RECOMBINE_G1(&cs1, &cs2, &clientSecret);
        if (res)
        {
            throw CryptoError("MPIN_RECOMBINE_G1", res);
        }

        return clientSecret;
    }

    SokData::SokData() {}

    SokData::SokData(const std::string & _iv, const std::string & c, const std::string & t) : iv(_iv), ciphertext(c), tag(t) {}

    SokData Crypto::SokEncrypt(const std::string& message, const std::string & sokSendKey, const std::string& userIdFrom, const std::string & userIdTo)
    {
        CreateRngOnce();

        if (sokSendKey.size() != G1S)
        {
            throw CryptoError(fmt::sprintf("Invalid sokSendKey size(%d) passed to SokEncrypt(). Must be %d",
                static_cast<int>(sokSendKey.size()), static_cast<int>(G1S)));
        }

        Octet aKeyG1(sokSendKey);
        Octet idTo(userIdTo);
        Octet encryptionKey(PAS);

        int res = SOK_PAIR1(HASH_TYPE_MPIN, 0, &aKeyG1, NULL, &idTo, &encryptionKey);
        if (res)
        {
            throw CryptoError("SOK_PAIR1", res);
        }

        Octet iv(PIV);
        FillWithRandomData(iv, m_rng);

        Octet idFrom(userIdFrom);
        Octet plaintext(message);
        Octet ciphertext(message.size());
        Octet tag(PTAG);

        SOK_AES_GCM_ENCRYPT(&encryptionKey, &iv, &idFrom, &plaintext, &ciphertext, &tag);

        return SokData(iv, ciphertext, tag);
    }

    std::string Crypto::SokDecrypt(const SokData & data, const std::string & sokRecvKey, const std::string& userIdFrom)
    {
        if (sokRecvKey.size() != G2S)
        {
            throw CryptoError(fmt::sprintf("Invalid sokRecvKey size(%d) passed to SokDecrypt(). Must be %d",
                static_cast<int>(sokRecvKey.size()), static_cast<int>(G2S)));
        }

        Octet bKeyG2(sokRecvKey);
        Octet idFrom(userIdFrom);
        Octet decriptionKey(PAS);

        int res = SOK_PAIR2(HASH_TYPE_MPIN, 0, &bKeyG2, NULL, &idFrom, &decriptionKey);
        if (res)
        {
            throw CryptoError("SOK_PAIR2", res);
        }

        Octet iv(data.iv);
        Octet ciphertext(data.ciphertext);
        Octet plaintext(data.ciphertext.size());
        Octet tag(PTAG);

        SOK_AES_GCM_DECRYPT(&decriptionKey, &iv, &idFrom, &ciphertext, &plaintext, &tag);

        if (data.tag != std::string(tag))
        {
            throw CryptoError("SokDecrypt() failed: tag mismatch");
        }

        return plaintext;
    }
}