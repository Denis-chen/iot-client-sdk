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
        String GetUserId() const;

        String mpinId;
        String clientSecret;
        StringVector dtaList;
    };

    class EventListener
    {
    public:
        virtual ~EventListener() {}
        virtual void OnAuthenticated() = 0;
        virtual void OnConnected() = 0;
        virtual void OnConnectionLost(const String& error) = 0;
        virtual void OnError(const String& error) = 0;
        virtual void OnMessageArrived(const String& topic, const String& payload) = 0;
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
        bool RunMessageLoop(unsigned long timeout);

    private:
        class Impl;
        Impl *m_impl;
    };
}

#endif // _IOT_CLIENT_H_
