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
    }

    Identity::Identity() {}

    Identity::Identity(const String & mpinIdHex, const String & clientSecretHex)
        : mpinId(HexDecode(mpinIdHex)), clientSecret(HexDecode(clientSecretHex)) {}

    String Identity::GetUserId() const
    {
        return (const json::String&) json::ConstElement(json::Parse(mpinId))["userID"];
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

    namespace
    {
        class DefaultEventListener : public EventListener
        {
        public:
            virtual void OnAuthenticated() {}
            virtual void OnConnected() {}
            virtual void OnConnectionLost(const String& error) {}
            virtual void OnError(const String& error) {}
            virtual void OnMessageArrived(const String& topic, const String& payload) {}
        } defaultEventListener;
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
        Impl() : m_state(NO_SESSION), m_authenticated(false)
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
                m_client.SetId(m_conf.identity.GetUserId());
                m_client.SetBrokerAddress(Addr(m_conf.mqttTlsBrokerAddr, DEFAULT_MQTT_TLS_PORT));
                if (m_conf.mqttCommandTimeoutMillisec > 0)
                {
                    m_client.SetCommandTimeout(m_conf.mqttCommandTimeoutMillisec);
                }
                m_client.SetQoS(m_conf.useMqttQoS2 ? MQTT::QOS2 : MQTT::QOS1);
                m_client.UsePersistentSession(m_conf.useMqttPersistentSession);

                m_state = INITIAL;
                CheckSate();
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
            if (!CheckSate())
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
            if (!CheckSate())
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
            if (!CheckSate())
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

        bool RunMessageLoop(unsigned long timeout)
        {
            Countdown timer(timeout);
            if (!CheckSate())
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
            GetEventListener().OnMessageArrived(topic, payload);
        }

    private:
        EventListener& GetEventListener()
        {
            return m_conf.GetEventListener();
        }

        bool CheckSate()
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
                m_client.SetPsk(authResult.sharedSecret, authResult.clientId);
                GetEventListener().OnAuthenticated();
                return true;
            }
            catch (Exception& e)
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

        const String& GetLastError() const
        {
            return m_lastError.empty() ? m_client.GetLastError() : m_lastError;
        }

        Config m_conf;
        MPinFull m_authenticator;
        bool m_authenticated;
        MqttTlsClient m_client;
        enum State { NO_SESSION, INITIAL, CONNECTED, DISCONNECTED } m_state;
        std::set<String> m_subscriptions;
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

    bool Client::RunMessageLoop(unsigned long timeout)
    {
        return m_impl->RunMessageLoop(timeout);
    }
}
