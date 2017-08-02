#include <iot/client.h>
#include "mqtt_tls_client.h"
#include "mpin_full.h"
#include "exception.h"
#include "utils.h"
#include <fmt/format.h>
#include <set>

namespace iot
{
    namespace
    {
        const char DEFAULT_MQTT_TLS_PORT[] = "8443";

        class DefaultEventListener : public EventListener
        {
        public:
            virtual void OnAuthenticated() {}
            virtual void OnIdentityChanged(const Identity& newIdentity) {}
            virtual void OnConnected() {}
            virtual void OnConnectionLost(const String& error) {}
            virtual void OnError(const String& error) {}
            virtual void OnMessageArrived(const String& topic, const String& payload) {}
            virtual void OnPrivateMessageArrived(const String& userIdFrom, const String& payload) {}
        } defaultEventListener;

        String GetPrivateMessageTopic(const String& userId)
        {
            return HexEncode(userId) + "/pm";
        }
    }

    Identity::Identity() {}

    Identity::Identity(const String & mpinIdHex, const String & clientSecretHex)
        : mpinId(HexDecode(mpinIdHex)), clientSecret(HexDecode(clientSecretHex)) {}

    void Identity::SetSokKeys(const String & sendKeyHex, const String & recvKeyHex)
    {
        sokSendKey = HexDecode(sendKeyHex);
        sokRecvKey = HexDecode(recvKeyHex);
    }

    String Identity::GetUserId() const
    {
        try
        {
            return (const json::String&) json::ConstElement(json::Parse(mpinId))["userID"];
        }
        catch (const json::Exception&)
        {
            return "";
        }
    }

    String Identity::GetMPinIdHex() const
    {
        return HexEncode(mpinId);
    }

    String Identity::GetClientSecretHex() const
    {
        return HexEncode(clientSecret);
    }

    String Identity::GetSokSendKeyHex() const
    {
        return HexEncode(sokSendKey);
    }

    String Identity::GetSokRecvKeyHex() const
    {
        return HexEncode(sokRecvKey);
    }

    Config::Config()
        : mqttCommandTimeoutMillisec(0), useMqttQoS2(true), useMqttPersistentSession(true)
    {
        ResetEventListener();
    }

    void Config::SetEventListener(EventListener & listener)
    {
        m_eventListener = &listener;
    }

    void Config::ResetEventListener()
    {
        m_eventListener = &defaultEventListener;
    }

    EventListener& Config::GetEventListener() const
    {
        return *m_eventListener;
    }

    class Client::Impl
    {
    public:
        Impl() : m_authenticator(m_crypto), m_authenticated(false), m_state(NO_SESSION)
        {
            MqttTlsClient::Handler handler;
            handler.attach(this, &Impl::OnMessageArrived);
            m_client.SetMessageHandler(handler);
        }

        void Configure(const Config& conf)
        {
            m_conf = conf;
        }

        Config& GetConfig()
        {
            return m_conf;
        }

        void StartSession()
        {
            if (!IsSessionStarted())
            {
                m_userId = m_conf.identity.GetUserId();
                m_privateMessagesTopic = GetPrivateMessageTopic(m_userId);

                m_client.SetId(m_userId);
                m_client.SetBrokerAddress(net::Addr(m_conf.mqttTlsBrokerAddr, DEFAULT_MQTT_TLS_PORT));
                if (m_conf.mqttCommandTimeoutMillisec > 0)
                {
                    m_client.SetCommandTimeout(m_conf.mqttCommandTimeoutMillisec);
                }
                m_client.SetQoS(m_conf.useMqttQoS2 ? MQTT::QOS2 : MQTT::QOS1);
                m_client.UsePersistentSession(m_conf.useMqttPersistentSession);

                m_state = INITIAL;
                CheckState();
            }
        }

        void EndSession()
        {
            if (IsSessionStarted())
            {
                m_client.Disconnect();
                m_subscriptions.clear();
                m_state = NO_SESSION;
            }
        }

        bool IsSessionStarted() const
        {
            return m_state != NO_SESSION;
        }

        bool IsConnected()
        {
            return m_client.IsConnected();
        }

        bool Subscribe(const String& topic)
        {
            if (!CheckState())
            {
                return false;
            }

            if (!m_client.Subscribe(topic))
            {
                GetEventListener().OnError(m_client.GetLastError());
                return false;
            }

            m_subscriptions.insert(topic);
            return true;
        }

        bool Unsubscribe(const String& topic)
        {
            if (!CheckState())
            {
                return false;
            }

            if (!m_client.Unsubscribe(topic))
            {
                GetEventListener().OnError(m_client.GetLastError());
                return false;
            }

            m_subscriptions.erase(topic);
            return true;
        }

        bool Publish(const String& topic, const String& payload)
        {
            if (!CheckState())
            {
                return false;
            }

            if (!m_client.Publish(topic, payload))
            {
                GetEventListener().OnError(m_client.GetLastError());
                return false;
            }

            return true;
        }

        bool ListenForPrivateMessages()
        {
            return Subscribe(m_privateMessagesTopic);
        }

        bool SendPrivateMessage(const String& userIdTo, const String& payload, bool encrypt)
        {
            try
            {
                return Publish(GetPrivateMessageTopic(userIdTo), SerializePrivateMessage(userIdTo, payload, encrypt));
            }
            catch (const Exception& e)
            {
                GetEventListener().OnError(fmt::sprintf("Failed to serialize private message: %s", e.what()));
                return false;
            }
        }

        bool RunMessageLoop(unsigned long timeout)
        {
            Countdown timer(timeout);
            if (!CheckState())
            {
                if (!timer.expired())
                {
                    timeout = timer.left_ms();
                    timespec t = { timeout / 1000, (timeout % 1000) * 1000000 };
                    nanosleep(&t, NULL);
                }
                return false;
            }

            if (!m_client.RunMessageLoop(timeout))
            {
                GetEventListener().OnError(m_client.GetLastError());
                return false;
            }

            return true;
        }

        void OnMessageArrived(MQTT::MessageData& md)
        {
            std::string topic(md.topicName.lenstring.data, md.topicName.lenstring.len);
            std::string payload((char *)md.message.payload, md.message.payloadlen);
            EventListener& el = GetEventListener();
            if (topic == m_privateMessagesTopic)
            {
                try
                {
                    PrivateMessage pm(*this, payload);
                    el.OnPrivateMessageArrived(pm.userIdFrom, pm.payload);
                }
                catch (const Exception& e)
                {
                    el.OnError(fmt::sprintf("Failed to deserialize private message: %s. Received payload: %s", e.what(), payload));
                }
            }
            else
            {
                el.OnMessageArrived(topic, payload);
            }
        }

    private:
        EventListener& GetEventListener()
        {
            return m_conf.GetEventListener();
        }

        bool CheckState()
        {
            switch (m_state)
            {
            case NO_SESSION:
                GetEventListener().OnError("No session started");
                return false;
            case INITIAL:
                if (!InitialConnect())
                {
                    GetEventListener().OnError(GetLastError());
                    return false;
                }
                m_state = CONNECTED;
                GetEventListener().OnConnected();
                return true;
            case CONNECTED:
            case DISCONNECTED:
                if (!IsConnected())
                {
                    if (m_state == CONNECTED)
                    {
                        m_state = DISCONNECTED;
                        GetEventListener().OnConnectionLost(GetLastError());
                    }

                    if (!Reconnect())
                    {
                        GetEventListener().OnError(GetLastError());
                    }
                    else
                    {
                        m_state = CONNECTED;
                        GetEventListener().OnConnected();
                    }
                }
                return m_state == CONNECTED;
            }
        }

        bool Authenticate()
        {
            if (m_authenticated)
            {
                return true;
            }

            try
            {
                m_lastError.clear();
                AuthResult authResult = m_authenticator.Authenticate(m_conf.authServerUrl, m_conf.identity);
                if (authResult.identityChanged)
                {
                    m_conf.identity = authResult.newIdentity;
                    GetEventListener().OnIdentityChanged(authResult.newIdentity);
                }

                m_client.SetPsk(authResult.sharedSecret, HexEncode(authResult.clientId));
                GetEventListener().OnAuthenticated();
                return true;
            }
            catch (const Exception& e)
            {
                m_lastError = fmt::sprintf("Authentication failed: %s", e.what());
                return false;
            }
        }

        bool InitialConnect()
        {
            if (Authenticate() && m_client.Connect())
            {
                return true;
            }
            return false;
        }

        bool Reconnect()
        {
            if (Authenticate() && m_client.Reconnect())
            {
                if (!m_client.IsSessionPresent())
                {
                    return RestoreSubscriptions();
                }
                return true;
            }
            return false;
        }

        bool RestoreSubscriptions()
        {
            for (std::set<String>::iterator s = m_subscriptions.begin(); s != m_subscriptions.end(); ++s)
            {
                if (!m_client.Subscribe(*s))
                {
                    GetEventListener().OnError(m_client.GetLastError());
                    return false;
                }
            }
            return true;
        }

        String SerializePrivateMessage(const String& userIdTo, const String& payload, bool encrypt)
        {
            json::Object json;
            json["from"] = json::String(m_userId);
            json["encrypted"] = json::Boolean(encrypt);
            if (encrypt)
            {
                SokData sok = m_crypto.SokEncrypt(payload, m_conf.identity.sokSendKey, m_userId, userIdTo);
                json["iv"] = json::String(HexEncode(sok.iv));
                json["ciphertext"] = json::String(HexEncode(sok.ciphertext));
                json["tag"] = json::String(HexEncode(sok.tag));
            }
            else
            {
                json["data"] = json::String(payload);
            }
            return json::ToString(json);
        }

        class PrivateMessage
        {
        public:
            PrivateMessage(Impl& client, const String& serializedData)
            {
                try
                {
                    json::ConstElement json = json::Parse(serializedData);
                    userIdFrom = (const json::String&) json["from"];
                    bool encrypted = ((const json::Boolean&) json["encrypted"]).Value();
                    if (encrypted)
                    {
                        SokData sok;
                        sok.iv = HexDecode((const json::String&) json["iv"]);
                        sok.ciphertext = HexDecode((const json::String&) json["ciphertext"]);
                        sok.tag = HexDecode((const json::String&) json["tag"]);
                        payload = client.m_crypto.SokDecrypt(sok, client.m_conf.identity.sokRecvKey, userIdFrom);

                        if (sok.iv.empty() || sok.ciphertext.empty() || sok.tag.empty())
                        {
                            throw JsonError(fmt::sprintf("Invalid private message parameters in message %s", serializedData));
                        }
                    }
                    else
                    {
                        payload = (const json::String&) json["data"];
                    }
                }
                catch (const json::Exception& e)
                {
                    throw JsonError(e.what());
                }
            }

            String userIdFrom;
            String payload;
        };

        const String& GetLastError() const
        {
            return m_lastError.empty() ? m_client.GetLastError() : m_lastError;
        }

        Config m_conf;
        Crypto m_crypto;
        MPinFull m_authenticator;
        bool m_authenticated;
        MqttTlsClient m_client;
        enum State { NO_SESSION, INITIAL, CONNECTED, DISCONNECTED } m_state;
        std::set<String> m_subscriptions;
        String m_userId;
        String m_privateMessagesTopic;
        String m_lastError;
    };

    Client::Client() : m_impl(new Impl()) {}

    Client::~Client()
    {
        delete m_impl;
    }

    void Client::Configure(const Config & conf)
    {
        m_impl->Configure(conf);
    }

    Config & Client::GetConfig()
    {
        return m_impl->GetConfig();
    }

    void Client::StartSession()
    {
        m_impl->StartSession();
    }

    void Client::EndSession()
    {
        m_impl->EndSession();
    }

    bool Client::IsSessionStarted() const
    {
        return m_impl->IsSessionStarted();
    }

    bool Client::IsConnected()
    {
        return m_impl->IsConnected();
    }

    bool Client::Subscribe(const String & topic)
    {
        return m_impl->Subscribe(topic);
    }

    bool Client::Unsubscribe(const String & topic)
    {
        return m_impl->Unsubscribe(topic);
    }

    bool Client::Publish(const String & topic, const String & payload)
    {
        return m_impl->Publish(topic, payload);
    }

    bool Client::ListenForPrivateMessages()
    {
        return m_impl->ListenForPrivateMessages();
    }

    bool Client::SendPrivateMessage(const String & userIdTo, const String & payload, bool encrypt)
    {
        return m_impl->SendPrivateMessage(userIdTo, payload, encrypt);
    }

    bool Client::RunMessageLoop(unsigned long timeout)
    {
        return m_impl->RunMessageLoop(timeout);
    }
}
