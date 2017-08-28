#ifndef _IOT_MQTT_TLS_CLIENT_H_
#define _IOT_MQTT_TLS_CLIENT_H_

#include <net/tls_connection.h>
#include "timer.h"
#define MQTTCLIENT_QOS2 1
#define MAX_INCOMING_QOS2_MESSAGES 10
#ifdef _WIN32
#define __PRETTY_FUNCTION__ __func__
#endif
#include <string.h>
#include <MQTTClient.h>
#include <string>

namespace iot
{
    class MqttTlsClient
    {
    public:
        class ConnectionAdapter : public net::TlsConnection
        {
        public:
            int read(unsigned char* buffer, int len, int timeoutMillisec);
            int write(const unsigned char* buffer, int len, int timeoutMillisec);
        };

        class TimerAdapter : public Timer
        {
        public:
            TimerAdapter();
            TimerAdapter(int ms);
            void countdown_ms(int ms);
            void countdown(int seconds);
            int left_ms();
            bool expired();
        };

        typedef MQTT::Client<ConnectionAdapter, TimerAdapter, 1024, 0> MqttClient;
        typedef MqttClient::messageHandler Handler;

        MqttTlsClient();
        void SetId(const std::string& clientId);
        const std::string& GetId() const;
        void SetBrokerAddress(const net::Addr& addr);
        void SetPsk(const std::string& psk, const std::string& pskId);
        void SetMessageHandler(const Handler & handler);
        void SetCommandTimeout(unsigned long timeoutMillisec);
        void SetQoS(MQTT::QoS qos);
        void UsePersistentSession(bool usePersistentSession);
        bool Connect();
        bool Reconnect();
        void Disconnect();
        bool IsConnected();
        bool IsSessionPresent() const;
        bool Subscribe(const std::string& topic);
        bool Unsubscribe(const std::string& topic);
        bool Publish(const std::string& topic, const std::string& message);
        bool RunMessageLoop(unsigned long timeoutMillisec);
        std::string GetCiphersuite() const;
        const std::string& GetLastError() const;

    protected:
        bool Connect(bool cleanSession);
        bool MqttConnect(bool cleanSession);
        bool OnError(const std::string& error);
        bool OnError(const std::string& error, const std::string& reason);

        ConnectionAdapter m_connection;
        MqttClient m_client;
        std::string m_clientId;
        MQTT::QoS m_qos;
        bool m_usePersistentSession;
        std::string m_lastError;
        bool m_sessionPresent;
    };
}

#endif // _IOT_MQTT_TLS_CLIENT_H_
