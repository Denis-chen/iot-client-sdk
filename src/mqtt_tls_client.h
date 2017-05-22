#ifndef _IOT_MQTT_TLS_CLIENT_H_
#define _IOT_MQTT_TLS_CLIENT_H_

#define MQTTCLIENT_QOS2 1
#define MAX_INCOMING_QOS2_MESSAGES 10
#include <string.h>
#include <MQTTClient.h>
#include <linux/linux.cpp>
#include "tls.h"
#include <string>

namespace iot
{
    class MqttTlsClient
    {
    public:
        typedef MQTT::Client<TlsConnection, Countdown, 1024, 0> MqttClient;
        typedef MqttClient::messageHandler Handler;

        MqttTlsClient();
        void SetId(const std::string& clientId);
        const std::string& GetId() const;
        void SetBrokerAddress(const Addr& addr);
        void SetPsk(const std::string& psk, const std::string& pskId);
        void SetMessageHandler(const Handler & handler);
        void SetCommandTimeout(unsigned long timeoutMillisec);
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

        TlsConnection m_connection;
        MqttClient m_client;
        std::string m_clientId;
        std::string m_lastError;
        bool m_sessionPresent;
    };
}

#endif // _IOT_MQTT_TLS_CLIENT_H_
