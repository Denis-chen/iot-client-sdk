#include "mqtt_tls_client.h"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace iot
{
    MqttTlsClient::MqttTlsClient() : m_client(m_connection), m_sessionPresent(false) {}

    void MqttTlsClient::SetId(const std::string & clientId)
    {
        m_clientId = clientId;
        m_connection.SetPersonalizationData(clientId);
    }

    const std::string & MqttTlsClient::GetId() const
    {
        return m_clientId;
    }

    void MqttTlsClient::SetBrokerAddress(const Addr & addr)
    {
        m_connection.SetAddress(addr);
    }

    void MqttTlsClient::SetPsk(const std::string & psk, const std::string& pskId)
    {
        m_connection.SetPsk(psk, pskId);
    }

    void MqttTlsClient::SetMessageHandler(const Handler & handler)
    {
        m_client.setDefaultMessageHandler(handler);
    }

    void MqttTlsClient::SetCommandTimeout(unsigned long timeoutMillisec)
    {
        m_client.setCommandTimeout(timeoutMillisec);
    }

    bool MqttTlsClient::Connect(bool cleanSession)
    {
        if (IsConnected())
        {
            return true;
        }

        if (m_connection.Connect() != 0)
        {
            return OnError(fmt::sprintf("Failed to connect to %s. Reason: %s", m_connection.GetAddress(), m_connection.GetLastError()));
        }

        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
        data.MQTTVersion = 3;
        data.clientID.lenstring.len = m_clientId.length();
        data.clientID.lenstring.data = const_cast<char *>(m_clientId.c_str());
        data.cleansession = cleanSession;
        if (m_client.connect(data, m_sessionPresent) != 0)
        {
            m_connection.Close();
            return OnError("Failed to connect MQTT client");
        }

        m_lastError.clear();
        return true;
    }

    void MqttTlsClient::Disconnect()
    {
        if (m_client.isConnected())
        {
            m_client.disconnect();
        }
        m_connection.Close();
        m_sessionPresent = false;
    }

    bool MqttTlsClient::IsConnected()
    {
        if (!m_connection.IsConnected())
        {
            if (m_client.isConnected())
            {
                m_client.disconnect();
                m_sessionPresent = false;
            }
            return false;
        }

        if (!m_client.isConnected())
        {
            if (m_connection.IsConnected())
            {
                m_connection.Close();
                m_sessionPresent = false;
            }
            return false;
        }

        return true;
    }

    bool MqttTlsClient::IsSessionPresent() const
    {
        return m_sessionPresent;
    }

    bool MqttTlsClient::Subscribe(const std::string & topic)
    {
        if (m_client.subscribe(topic.c_str(), MQTT::QOS2, Handler()) != 0)
        {
            return OnError(fmt::sprintf("Failed to subscribe MQTT client to %s topic", topic));
        }

        return true;
    }

    bool MqttTlsClient::Unsubscribe(const std::string & topic)
    {
        if (m_client.unsubscribe(topic.c_str()) != 0)
        {
            return OnError(fmt::sprintf("Failed to unsubscribe MQTT client from %s topic", topic));
        }
        
        return true;
    }

    bool MqttTlsClient::Publish(const std::string & topic, const std::string & message)
    {
        MQTT::Message m;
        m.qos = MQTT::QOS2;
        m.retained = false;
        m.dup = false;
        m.payload = const_cast<char *>(message.c_str());
        m.payloadlen = message.length();

        if (m_client.publish(topic.c_str(), m) != 0)
        {
            return OnError(fmt::sprintf("Failed to publish message to %s topic", topic));
        }

        return true;
    }

    bool MqttTlsClient::RunMessageLoop(unsigned long timeoutMillisec)
    {
        //int res = m_client.yield(timeoutMillisec);
        //if (res < 0)
        //{
        //    if (!m_connection.IsLastIoTimedOut())
        //    {
        //        return OnError(fmt::sprintf("MQTT message loop failed with code %d", res));
        //    }
        //}

        // TODO: Investigate the reason for false error printing here
        m_client.yield(timeoutMillisec);

        return true;
    }

    std::string MqttTlsClient::GetCiphersuite() const
    {
        return m_connection.GetCiphersuite();
    }

    const std::string & MqttTlsClient::GetLastError() const
    {
        return m_lastError;
    }

    bool MqttTlsClient::OnError(const std::string& error)
    {
        m_lastError = error;
        m_sessionPresent = false;
        return false;
    }
}
