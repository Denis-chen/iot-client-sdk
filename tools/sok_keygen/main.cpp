extern "C"
{
#include <sok.h>
#include <randapi.h>
}
#include <json.h>
#include <fmt/format.h>
#include <iostream>
#include <string>
#include <fstream>

using std::cout;
using std::endl;
using std::string;

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
        Octet(const Octet& other);
        Octet& operator=(const Octet& other);

        bool m_freeStorage;
    };

    const int G1S = 2 * PFS + 1;
    const int G2S = 4 * PFS;

    bool GenerateRandomSeed(char *buf, size_t len)
    {
        if (len == 0)
        {
            return true;
        }

        size_t rc = 0;
        FILE *fp = fopen("/dev/urandom", "rb");
        if (fp != NULL)
        {
            rc = fread(buf, 1, len, fp);
            fclose(fp);
        }

        return rc == len;
    }

    const char* hexChars = "0123456789abcdef";

    std::string HexEncode(const std::string & data)
    {
        std::string result;
        result.reserve(2 * data.length());
        for (std::string::const_iterator i = data.begin(); i != data.end(); ++i)
        {
            unsigned char c = *i;
            char c1 = hexChars[c >> 4];   // High nibble
            char c2 = hexChars[c & 0x0F]; // Low nibble	
            result += c1;
            result += c2;
        }
        return result;
    }

    std::string HexDecode(const std::string & data)
    {
        size_t len = data.length();
        if (len % 2 != 0)
        {
            return "";
        }

        std::string result;
        result.resize(len / 2);

        for (size_t i = 0; i < len; ++i)
        {
            char c = tolower(data[i]);
            uint8_t nibble = 0;

            switch (c)
            {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                nibble = c - '0';
                break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                nibble = c - 'a' + 0xA;
                break;
            default:
                return "";
            }

            if (i % 2 == 0)
            {
                result[i / 2] = (nibble << 4);
            }
            else
            {
                result[i / 2] |= nibble;
            }
        }

        return result;
    }

    const string masterSecretFile = "master_secret.json";

    class MasterSecret
    {
    public:
        bool Generate()
        {
            csprng rng;
            Octet seed(100);
            seed.len = seed.max;
            if (!GenerateRandomSeed(seed.val, seed.len))
            {
                cout << "ERROR: Failed to init random generator" << endl;
                return false;
            }
            CREATE_CSPRNG(&rng, &seed);

            Octet masterSecret1(PGS);
            Octet masterSecret2(PGS);

            SOK_RANDOM_GENERATE(&rng, &masterSecret1);
            SOK_RANDOM_GENERATE(&rng, &masterSecret2);

            KILL_CSPRNG(&rng);

            miracl = masterSecret1;
            customer = masterSecret2;

            return true;
        }

        bool LoadFromFile()
        {
            std::ifstream file(masterSecretFile.c_str(), std::fstream::in);
            if (!file.is_open())
            {
                cout << fmt::sprintf("ERROR: Failed to open %s master secret file for reading", masterSecretFile) << endl;
                return false;
            }

            try
            {
                json::ConstElement json = json::Parse(file);
                miracl = HexDecode((const json::String&) json["miracl"]);
                customer = HexDecode((const json::String&) json["customer"]);
                return true;
            }
            catch (json::Exception& e)
            {
                cout << fmt::sprintf("ERROR: Failed to parse %s master secret file: %s", masterSecretFile, e.what()) << endl;
                return false;
            }
        }

        bool SaveToFile()
        {
            std::ofstream file(masterSecretFile.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open())
            {
                cout << fmt::sprintf("ERROR: Failed to open %s master secret file for writing", masterSecretFile) << endl;
                return false;
            }

            json::Object json;
            json["miracl"] = json::String(HexEncode(miracl));
            json["customer"] = json::String(HexEncode(customer));

            file << json;

            cout << fmt::sprintf("Master secret saved to %s", masterSecretFile) << endl;
            return true;
        }

        string miracl;
        string customer;
    };
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "Usage:" << endl;
        cout << "   sok_keygen <user_id>" << endl;
        cout << "or:" << endl;
        cout << "   sok_keygen -generateMasterSecret" << endl;
        return 0;
    }

    MasterSecret masterSecret;
    if (string(argv[1]) == "-generateMasterSecret")
    {
        bool res = masterSecret.Generate() && masterSecret.SaveToFile();
        return res ? 0 : -1;
    }

    if (!masterSecret.LoadFromFile())
    {
        cout << "Failed to load master secret. Run ./sok_keygen -generateMasterSecret to generate one" << endl;
        return -1;
    }

    Octet masterSecret1(masterSecret.miracl);
    Octet masterSecret2(masterSecret.customer);

    int res = 0;
    string userId = argv[1];

    Octet id(userId);
    Octet hcid(PFS);
    SOK_HASH_ID(SHA256, &id, &hcid);

    Octet g1Secret1(G1S);
    Octet g1Secret2(G1S);
    SOK_GET_G1_SECRET(&masterSecret1, &hcid, &g1Secret1);
    SOK_GET_G1_SECRET(&masterSecret2, &hcid, &g1Secret2);

    Octet resKeyG1(G1S);
    res = SOK_RECOMBINE_G1(&g1Secret1, &g1Secret2, &resKeyG1);
    if (res)
    {
        cout << "ERROR: SOK_RECOMBINE_G1 failed" << endl;
    }

    Octet g2Secret1(G2S);
    Octet g2Secret2(G2S);
    SOK_GET_G2_SECRET(&masterSecret1, &hcid, &g2Secret1);
    SOK_GET_G2_SECRET(&masterSecret2, &hcid, &g2Secret2);

    Octet resKeyG2(G2S);
    res = SOK_RECOMBINE_G2(&g2Secret1, &g2Secret2, &resKeyG2);
    if (res)
    {
        cout << "ERROR: SOK_RECOMBINE_G2 failed" << endl;
    }

    json::Object keys;
    keys["sokSendKey"] = json::String(HexEncode(resKeyG1));
    keys["sokRecvKey"] = json::String(HexEncode(resKeyG2));

    cout << keys << endl;

    return 0;
}
