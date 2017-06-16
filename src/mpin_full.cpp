#include "mpin_full.h"
#include "utils.h"
#include "exception.h"
#include <fmt/format.h>
#include <sstream>

namespace iot
{
    MPinFull::MPinFull(Crypto & crypto) : m_crypto(crypto) {}

    AuthResult MPinFull::Authenticate(const std::string& server, const Identity & id)
    {
        try
        {
            return DoAuth(server, id);
        }
        catch (json::Exception& e)
        {
            throw JsonError(e.what());
        }
    }

    AuthResult MPinFull::DoAuth(const std::string& server, const Identity & id)
    {
        AuthResult res;
        json::Object request;
        json::ConstElement response;
        std::string mpinIdHex = HexEncode(id.mpinId);

        res.clientId = m_crypto.HashId(id.mpinId);
        Pass1Data pass1 = m_crypto.Client1(id.mpinId, id.clientSecret);

        request["dta"] = json::ToArray(id.dtaList.begin(), id.dtaList.end());
        request["mpin_id"] = json::String(mpinIdHex);
        request["U"] = json::String(HexEncode(pass1.u));
        request["UT"] = json::String(HexEncode(pass1.ut));

        response = MakeRequest(fmt::sprintf("%s/auth/pass1", server), request);

        Pass2Data pass2;
        pass2.y = HexDecode((const json::String&) response["y"]);
        pass2.v = m_crypto.Client2(pass1.x, pass2.y, pass1.sec);
        pass2.z = m_crypto.GetG1Multiple(res.clientId, pass2.r);

        request.Clear();
        request["mpin_id"] = json::String(mpinIdHex);
        request["WID"] = json::String("");
        request["OTP"] = json::Boolean(false);
        request["V"] = json::String(HexEncode(pass2.v));
        request["Z"] = json::String(HexEncode(pass2.z));

        response = MakeRequest(fmt::sprintf("%s/auth/pass2", server), request);

        request.Clear();
        request["mpinResponse"]["authOTT"] = response["authOTT"];

        response = MakeRequest(fmt::sprintf("%s/auth/authenticate", server), request);

        AuthData auth;
        auth.t = HexDecode((const json::String&) response["T"]);
        auth.hm = m_crypto.HashAll(res.clientId, pass1, pass2, auth.t);
        auth.precomp = m_crypto.Precompute(id.clientSecret, res.clientId);

        res.sharedSecret = m_crypto.ClientKey(pass1, pass2, auth);
        return res;
    }
}
