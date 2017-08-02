#include "mqtt_tls_client.h"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace iot
{
    int MqttTlsClient::TlsConnection::read(unsigned char * buffer, int len, int timeoutMillisec)
    {
        return ReadAll(buffer, len, timeoutMillisec);
    }

    int MqttTlsClient::TlsConnection::write(const unsigned char * buffer, int len, int timeoutMillisec)
    {
        return Write(buffer, len, timeoutMillisec);
    }

    MqttTlsClient::MqttTlsClient()
        : m_client(m_connection), m_qos(MQTT::QOS2), m_usePersistentSession(true), m_sessionPresent(false) {}

    void MqttTlsClient::SetId(const std::string & clientId)
    {
        m_clientId = clientId;
        m_connection.SetPersonalizationData(clientId);
    }

    const std::string & MqttTlsClient::GetId() const
    {
        return m_clientId;
    }

    void MqttTlsClient::SetBrokerAddress(const net::Addr & addr)
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

    void MqttTlsClient::SetQoS(MQTT::QoS qos)
    {
        m_qos = qos;
    }

    void MqttTlsClient::UsePersistentSession(bool usePersistentSession)
    {
        m_usePersistentSession = usePersistentSession;
    }

    bool MqttTlsClient::Connect()
    {
        if (!Connect(true))
        {
            return false;
        }

        if (!m_usePersistentSession)
        {
            return true;
        }

        Disconnect();
        return Connect(false);
    }

    bool MqttTlsClient::Reconnect()
    {
        if (!m_usePersistentSession)
        {
            return Connect(true);
        }

        return Connect(false);
    }

    bool MqttTlsClient::Connect(bool cleanSession)
    {
        if (IsConnected())
        {
            return true;
        }

        if (m_connection.Connect() != 0)
        {
            return OnError(fmt::sprintf("Failed to connect to %s", m_connection.GetAddress()));
        }

        if (!MqttConnect(cleanSession))
        {
            m_connection.Close();
            return false;
        }

        m_lastError.clear();
        return true;
    }

    bool MqttTlsClient::MqttConnect(bool cleanSession)
    {
        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
        data.MQTTVersion = 4;
        data.clientID.lenstring.len = m_clientId.length();
        data.clientID.lenstring.data = const_cast<char *>(m_clientId.c_str());
        data.cleansession = cleanSession;
        if (m_client.connect(data, m_sessionPresent) != 0)
        {
            return OnError("Failed to connect MQTT client");
        }
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
        if (m_client.subscribe(topic.c_str(), m_qos, Handler()) != 0)
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
        m.qos = m_qos;
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
        int res = m_client.yield(timeoutMillisec);
        if (res < 0)
        {
            // TODO: Check why sometimes the connection is OK, no timeout occured, but still yeld returns FAILURE
            //if (!m_connection.IsTimedOut())
            if (!m_connection.IsConnected())
            {
                return OnError(fmt::sprintf("MQTT message loop failed with code %d", res));
            }
            else
            {
                // TODO: Currently MQTT::Client::yield must be fixed to enable this check
                if (res == MQTT::BUFFER_OVERFLOW)
                {
                    return OnError(fmt::sprintf("MQTT message loop failed with code %d", res), "Buffer overflow");
                }
            }
        }
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
        std::string reason;
        if (m_connection.IsConnected())
        {
            reason = "MQTT::Client specific error";
        }
        else
        {
            reason = fmt::sprintf("%s", m_connection.GetLastError());
        }
        return OnError(error, reason);
    }

    bool MqttTlsClient::OnError(const std::string & error, const std::string & reason)
    {
        m_lastError = fmt::sprintf("%s. Reason: %s", error, reason);
        m_sessionPresent = false;
        return false;
    }
}
