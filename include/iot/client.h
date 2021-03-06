#ifndef _IOT_CLIENT_H_
#define _IOT_CLIENT_H_

#include <string>
#include <vector>

namespace iot
{
    typedef std::string String;
    typedef std::vector<String> StringVector;

    class Identity
    {
    public:
        Identity();
        Identity(const String& mpinIdHex, const String& clientSecretHex);
        void SetSokKeys(const String& sendKeyHex, const String& recvKeyHex);
        String GetUserId() const;
        String GetMPinIdHex() const;
        String GetClientSecretHex() const;
        String GetSokSendKeyHex() const;
        String GetSokRecvKeyHex() const;

        String mpinId;
        String clientSecret;
        StringVector dtaList;
        String sokSendKey;
        String sokRecvKey;
    };

    class EventListener
    {
    public:
        virtual ~EventListener() {}
        virtual void OnAuthenticated() = 0;
        virtual void OnIdentityChanged(const Identity& newIdentity) = 0;
        virtual void OnConnected() = 0;
        virtual void OnConnectionLost(const String& error) = 0;
        virtual void OnError(const String& error) = 0;
        virtual void OnMessageArrived(const String& topic, const String& payload) = 0;
        virtual void OnPrivateMessageArrived(const String& userIdFrom, const String& payload) = 0;
    };

    class Config
    {
    public:
        Config();
        void SetEventListener(EventListener& listener);
        void ResetEventListener();
        EventListener& GetEventListener() const;

        String authServerUrl;
        String mqttTlsBrokerAddr;
        unsigned long mqttCommandTimeoutMillisec;
        bool useMqttQoS2;
        bool useMqttPersistentSession;
        Identity identity;

    private:
        EventListener *m_eventListener;
    };

    class Client
    {
    public:
        Client();
        ~Client();
        void Configure(const Config& conf);
        Config& GetConfig();
        void StartSession();
        void EndSession();
        bool IsSessionStarted() const;
        bool IsConnected();
        bool Subscribe(const String& topic);
        bool Unsubscribe(const String& topic);
        bool Publish(const String& topic, const String& payload);
        bool ListenForPrivateMessages();
        bool SendPrivateMessage(const String& userIdTo, const String& payload, bool encrypt = true);
        bool RunMessageLoop(unsigned long timeout);

    private:
        class Impl;
        Impl *m_impl;
    };
}

#endif // _IOT_CLIENT_H_
