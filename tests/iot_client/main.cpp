#include "flags.h"
#include <iot/client.h>
#include <json.h>
#include <fmt/format.h>
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;
using iot::String;

const char CONFIG[] = "config";
static Flags::Option options[] =
{
    { CONFIG, "JSON Config file name", "tests/iot_client/iot_client.json" },
    { "help or h", "Prints usage info", "" },
};
static size_t optionsCount = sizeof(options) / sizeof(options[0]);

class Config : public iot::Config
{
public:
    std::string subscribeTopic;
    std::string publishTopic;

    bool Load(const std::string& fileName)
    {
        std::ifstream file(fileName.c_str(), std::fstream::in);
        if (!file.is_open())
        {
            cout << fmt::sprintf("Failed to open %s config file", fileName) << endl;
            return false;
        }

        try
        {
            Load(json::Parse(file));
            return true;
        }
        catch (json::Exception& e)
        {
            cout << fmt::sprintf("Failed to parse config file: %s", e.what()) << endl;
            return false;
        }
    }

private:
    void Load(const json::Object& json)
    {
        authServerUrl = (const json::String&) json["authServerUrl"];
        mqttTlsBrokerAddr = (const json::String&) json["mqttTlsBrokerAddr"];
        subscribeTopic = (const json::String&) json["subscribeTopic"];
        publishTopic = (const json::String&) json["publishTopic"];
        identity = LoadIdentity(json["identity"]);

        if (json.Find("mqttCommandTimeoutMillisec") != json.End())
        {
            mqttCommandTimeoutMillisec = (const json::Number&) json["mqttCommandTimeoutMillisec"];
        }
    }

    iot::Identity LoadIdentity(const json::Object& json)
    {
        iot::Identity id((const json::String&) json["mpinID"], (const json::String&) json["clientSecret"]);
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

int main(int argc, char *argv[])
{
    Flags flags(argc, argv, Flags::Options(options, options + optionsCount));
    if (flags.Exist("h") || flags.Exist("help"))
    {
        flags.PrintUsage();
        return 0;
    }

    Config conf;
    if (!conf.Load(flags.Get(CONFIG)))
    {
        flags.PrintUsage();
        return -1;
    }
    String userId = conf.identity.GetUserId();

    EventListener eventListener(conf);
    iot::Client client;

    client.Configure(conf);
    client.StartSession();

    bool subscribed = false;
    int counter = 0;
    while (++counter)
    {
        if (!subscribed)
        {
            if (client.Subscribe(conf.subscribeTopic))
            {
                subscribed = true;
                cout << "Subscribed to topic " << conf.subscribeTopic << endl;
            }
        }

        if (subscribed)
        {
            std::string message = fmt::sprintf("Test message from %s %d", userId, counter);
            if (client.Publish(conf.publishTopic, message))
            {
                cout << " * Published message '" << message << "' to " << conf.publishTopic << endl;
            }
        }

        client.RunMessageLoop(1000);
    }

    client.EndSession();

    return 0;
}
