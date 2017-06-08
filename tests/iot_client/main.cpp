#include "flags.h"
#include <iot/client.h>
#include <json.h>
#include <fmt/format.h>
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;
using iot::String;

namespace
{
    const char AUTH_SERVER_URL[] = "authServerUrl";
    const char IDENTITY_FILE[] = "identityFile";
    const char MQTT_BROCKER_ADDR[] = "mqttTlsBrokerAddr";
    const char MQTT_COMMAND_TIMEOUT[] = "mqttCommandTimeout";
    const char USE_MQTT_QOS2[] = "useMqttQoS2";
    const char USE_MQTT_PERSISTENT_SESSION[] = "useMqttPersistentSession";
    const char AWS_IOT_COMPLIANCE[] = "awsIoTCompliance";
    const char SUBSCRIBE_TO_TOPIC[] = "subscribeToTopic";
    const char PUBLISH_TO_TOPIC[] = "publishToTopic";
    const char PUBLISH_MESSAGE[] = "publishMessage";

    Flags::Option options[] =
    {
        { AUTH_SERVER_URL, "M-Pin Full authentication server URL", "http://127.0.0.1:8080" },
        { IDENTITY_FILE, "M-Pin Full identity JSON file", "tests/iot_client/identity.json" },
        { MQTT_BROCKER_ADDR, "Address of the MQTT TLS brocker", "127.0.0.1:8443" },
        { MQTT_COMMAND_TIMEOUT, "MQTT command timeout in milliseconds", "10000" },
        { USE_MQTT_QOS2, "If true, MQTT publish/subscribe will be made with QoS2, else with QoS1", "true" },
        { USE_MQTT_PERSISTENT_SESSION, "If true, persistent MQTT session will be requested when connecting", "true" },
        { AWS_IOT_COMPLIANCE, "Equivalent to useMqttQoS2=false and useMqttPersistentSession=false if true", "false" },
        { SUBSCRIBE_TO_TOPIC, "MQTT topic name to subscribe and continuously listen to, if specified", "" },
        { PUBLISH_TO_TOPIC, "MQTT topic name to publish a message to, if specified", "" },
        { PUBLISH_MESSAGE, "Message to publish. If empty, read from stdin until the first new line", "" },
        { "help or h", "Prints usage info", "" },
    };
    size_t optionsCount = sizeof(options) / sizeof(options[0]);

    class Config : public iot::Config
    {
    public:
        std::string subscribeTopic;
        std::string publishTopic;
        std::string publishMessage;

        bool Load(const Flags& flags)
        {
            authServerUrl = flags.Get(AUTH_SERVER_URL);

            mqttTlsBrokerAddr = flags.Get(MQTT_BROCKER_ADDR);
            mqttCommandTimeoutMillisec = atoi(flags.Get(MQTT_COMMAND_TIMEOUT).c_str());
            useMqttQoS2 = flags.GetBoolean(USE_MQTT_QOS2);
            useMqttPersistentSession = flags.GetBoolean(USE_MQTT_PERSISTENT_SESSION);
            if (flags.GetBoolean(AWS_IOT_COMPLIANCE))
            {
                cout << "Forcing AWS IoT compliance" << endl;
                useMqttQoS2 = false;
                useMqttPersistentSession = false;
            }

            subscribeTopic = flags.Get(SUBSCRIBE_TO_TOPIC);
            publishTopic = flags.Get(PUBLISH_TO_TOPIC);
            publishMessage = flags.Get(PUBLISH_MESSAGE);

            if (subscribeTopic.empty() && publishTopic.empty())
            {
                cout << fmt::sprintf("At least one of the %s or %s options must be specified", SUBSCRIBE_TO_TOPIC, PUBLISH_TO_TOPIC) << endl;
                return false;
            }

            std::string identityFileName = flags.Get(IDENTITY_FILE);
            std::ifstream file(identityFileName.c_str(), std::fstream::in);
            if (!file.is_open())
            {
                cout << fmt::sprintf("Failed to open %s identity file", identityFileName) << endl;
                return false;
            }

            try
            {
                identity = LoadIdentity(json::Parse(file));
                return true;
            }
            catch (json::Exception& e)
            {
                cout << fmt::sprintf("Failed to parse %s identity file: %s", identityFileName, e.what()) << endl;
                return false;
            }
        }

    private:
        iot::Identity LoadIdentity(const json::Object& json)
        {
            iot::Identity id((const json::String&) json["mpin_id"], (const json::String&) json["client_secret"]);
            const json::Array& dtaList = json["dta"];
            for (json::Array::const_iterator i = dtaList.Begin(); i != dtaList.End(); ++i)
            {
                id.dtaList.push_back((const json::String&) *i);
            }
            return id;
        }
    };

    class EventListener : public iot::EventListener
    {
    public:
        EventListener(Config& conf) : m_conf(conf)
        {
            conf.SetEventListener(*this);
        }

        virtual void OnAuthenticated()
        {
            cout << "Authenticated to " << m_conf.authServerUrl << endl;
        }

        virtual void OnConnected()
        {
            cout << "Connected to " << m_conf.mqttTlsBrokerAddr << endl;
        }

        virtual void OnConnectionLost(const String& error)
        {
            cout << "Connection lost: " << error << endl << " * Reconnecting..." << endl;
        }

        virtual void OnError(const String& error)
        {
            cout << "ERROR: " << error << endl;
        }

        virtual void OnMessageArrived(const String& topic, const String& payload)
        {
            cout << " - Incomming message (from " << topic << "): '" << payload << "'" << endl;
        }

    private:
        Config& m_conf;
    };
}

int main(int argc, char *argv[])
{
    Flags flags(argc, argv, Flags::Options(options, options + optionsCount));
    if (flags.Exist("h") || flags.Exist("help"))
    {
        flags.PrintUsage();
        return 0;
    }

    Config conf;
    if (!conf.Load(flags))
    {
        flags.PrintUsage();
        return -1;
    }

    if (!conf.publishTopic.empty())
    {
        if (conf.publishMessage.empty())
        {
            cout << "Enter message to publish:" << endl;
            std::getline(std::cin, conf.publishMessage);
        }
    }

    EventListener eventListener(conf);
    iot::Client client;
    client.Configure(conf);
    client.StartSession();

    if (!conf.publishMessage.empty())
    {
        bool published = false;
        while (!published)
        {
            published = client.Publish(conf.publishTopic, conf.publishMessage);
            client.RunMessageLoop(published ? 100 : 1000);
        }

        cout << " * Published message '" << conf.publishMessage << "' to " << conf.publishTopic << endl;
    }

    if (!conf.subscribeTopic.empty())
    {
        bool subscribed = false;
        while (true)
        {
            if (!subscribed)
            {
                if (client.Subscribe(conf.subscribeTopic))
                {
                    subscribed = true;
                    cout << "Subscribed to topic " << conf.subscribeTopic << endl;
                }
            }

            client.RunMessageLoop(1000);
        }
    }

    client.EndSession();
    return 0;
}
