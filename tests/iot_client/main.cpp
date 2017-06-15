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
    const char LISTEN_FOR_PMS[] = "listenForPms";
    const char SEND_PM_TO[] = "sendPmTo";

    const String sendPmToDesc =
        fmt::sprintf("User id to send -%s as a private message to (if specified)", PUBLISH_MESSAGE);

    Flags::Option options[] =
    {
        { AUTH_SERVER_URL, "M-Pin Full authentication server URL", "http://127.0.0.1:8080" },
        { IDENTITY_FILE, "M-Pin Full identity JSON file", "tests/iot_client/identity.json" },
        { MQTT_BROCKER_ADDR, "Address of the MQTT TLS brocker", "127.0.0.1:8443" },
        { MQTT_COMMAND_TIMEOUT, "MQTT command timeout in milliseconds", "10000" },
        { USE_MQTT_QOS2, "If true, MQTT publish/subscribe will be made with QoS2, else with QoS1", "true" },
        { USE_MQTT_PERSISTENT_SESSION, "If true, persistent MQTT session will be requested when connecting", "true" },
        { AWS_IOT_COMPLIANCE, "Force useMqttQoS2=false and useMqttPersistentSession=false if true", "false" },
        { SUBSCRIBE_TO_TOPIC, "MQTT topic name to subscribe and continuously listen to, if specified", "" },
        { PUBLISH_TO_TOPIC, "MQTT topic name to publish a message to, if specified", "" },
        { PUBLISH_MESSAGE, "Message to publish. If empty, read from stdin until the first new line", "" },
        { LISTEN_FOR_PMS, "Accept private messages (can be encrypted if sokRecvKey is set)", "true" },
        { SEND_PM_TO, "Send -publishMessage as private to the specified user (encrypted if sokSendKey is set)", "" },
        { "help or h", "Prints usage info", "" },
    };
    size_t optionsCount = sizeof(options) / sizeof(options[0]);

    class Config : public iot::Config
    {
    public:
        std::string subscribeTopic;
        std::string publishTopic;
        std::string publishMessage;
        bool listenForPms;
        std::string sendPmTo;

        Config() : iot::Config(), listenForPms(true) {}

        bool Load(const Flags& flags)
        {
            authServerUrl = flags.Get(AUTH_SERVER_URL);

            std::string identityFileName = flags.Get(IDENTITY_FILE);
            try
            {
                identity = LoadIdentity(ParseJsonFile(identityFileName));
            }
            catch (json::Exception& e)
            {
                cout << fmt::sprintf("Failed to parse '%s' identity file: %s", identityFileName, e.what()) << endl;
                return false;
            }

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
            listenForPms = flags.GetBoolean(LISTEN_FOR_PMS);
            sendPmTo = flags.Get(SEND_PM_TO);

            if (subscribeTopic.empty() && publishTopic.empty() && sendPmTo.empty() && !listenForPms)
            {
                cout << fmt::sprintf("At least one of the %s, %s, %s or %s options must be specified/set",
                    SUBSCRIBE_TO_TOPIC, PUBLISH_TO_TOPIC, SEND_PM_TO, LISTEN_FOR_PMS) << endl;
                return false;
            }

            return true;
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
            id.SetSokKeys(GetOptionalString(json, "sokSendKey"), GetOptionalString(json, "sokRecvKey"));
            return id;
        }

        std::string GetOptionalString(const json::Object& json, const std::string& name)
        {
            json::Object::const_iterator i = json.Find(name);
            if (i == json.End())
            {
                return "";
            }
            return (const json::String&) i->element;
        }

        json::UnknownElement ParseJsonFile(const std::string& fileName)
        {
            std::ifstream file(fileName.c_str(), std::fstream::in);
            if (!file.is_open())
            {
                throw json::Exception("Failed to open file");
            }
            return json::Parse(file);
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

        virtual void OnPrivateMessageArrived(const String& userIdFrom, const String& payload)
        {
            cout << " - Incomming private message (from " << userIdFrom << "): '" << payload << "'" << endl;
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

    if (!conf.publishTopic.empty() || !conf.sendPmTo.empty())
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

    if (!conf.publishTopic.empty())
    {
        bool published = false;
        while (!published)
        {
            published = client.Publish(conf.publishTopic, conf.publishMessage);
            client.RunMessageLoop(published ? 100 : 1000);
        }

        cout << " * Published message '" << conf.publishMessage << "' to " << conf.publishTopic << endl;
    }

    if (!conf.sendPmTo.empty())
    {
        bool encrypt = !conf.identity.sokSendKey.empty();
        bool published = false;
        while (!published)
        {
            published = client.SendPrivateMessage(conf.sendPmTo, conf.publishMessage, encrypt);
            client.RunMessageLoop(published ? 100 : 1000);
        }

        cout << " * Sent private message '" << conf.publishMessage << "' (encrypted=" << encrypt << ") to " << conf.sendPmTo << endl;
    }

    if (!conf.subscribeTopic.empty() || conf.listenForPms)
    {
        bool subscribed = conf.subscribeTopic.empty();
        bool subscribedForPms = !conf.listenForPms;

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

            if (!subscribedForPms)
            {
                if (client.ListenForPrivateMessages())
                {
                    subscribedForPms = true;
                    cout << "Started listening for private messages" << endl;
                }
            }

            client.RunMessageLoop(1000);
        }
    }

    client.EndSession();
    return 0;
}
