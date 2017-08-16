# MIRACL IoT Client SDK

The MIRACL IoT Client SDK is a library, which implements the MQTT protocol over secure TLS-PSK connection.
It uses the M-Pin Full protocol to authenticate to an M-Pin Full server and to negotiate Shared Session Key
on both client and server side. This key is used to establish a secure TLS-PSK connection to a TLS MQTT
broker. The library also implements private (end-to-end) messaging with additional SOK encryption.

## API

The interface of the client library is located in `include/iot/client.h`. It consists from the following
types:

- `Identity` - data to authenticate with to the M-Pin Full server. Contains `mpinId`, `clientSecret`, list
of hex-encoded `dta`-s, `sokSendKey` and `sokRecvKey`.
- `EventListener` - defines a callback interface for receiving events from the library. Library user should
inherit from this class to receive events. It has the following methods:
    - `void OnAuthenticated()` - invoked after successfull authentication.
    - `void OnConnected()` - invoked after successfully connected to the TLS MQTT broker.
    - `void OnConnectionLost(const String& error)` - invoked after the connection to the broker has been lost.
    - `void OnError(const String& error)` - invoked when error occurred.
    - `void OnMessageArrived(const String& topic, const String& payload)` - invoked when a message is published
to a topic, that this client has subscribed for.
    - `void OnPrivateMessageArrived(const String& userIdFrom, const String& payload)` - invoked when a private
message is sent to this client (is published to its private messages topic).
- `Config` - contains all the configuration properties of the library:
    - `authServerUrl` - M-Pin Full authentication server URL (`http://host:port/path`).
    - `identity` - `Identity` to authenticate with.
    - `mqttTlsBrokerAddr` - address of the TLS MQTT broker (`host:port`).
    - `mqttCommandTimeoutMillisec` - timeout for the MQTT commands (connect, publish, subscribe...).
    - `useMqttQoS2` flag - if set, MQTT publish and subscribe will be made with QoS 2, else with QoS 1.
    - `useMqttPersistentSession` flag - if set, persistent MQTT session will be requested when connecting.
    - `void SetEventListener(EventListener& listener)` - used to specify an `EventListener` callback.

    In order to connect the client to AWS Message Broker, useMqttQoS2 and useMqttPersistentSession must be set to false.

- `Client` - the main library class, which implements the MQTT commands. It has the following methods:
    - `void Configure(const Config& conf)` - sets all configuration properties at once.
    - `Config& GetConfig()` - returns a reference to the client configuration that can be used to set specific
properties.
    - `void StartSession()` - starts a new session (connection). To issue an MQTT command, you have to first
start a session. When a session is started, the client will authomatically authenticate and connect to the TLS
MQTT broker. During an active session, the connection will be automatically reestablished if lost. When initially
connecting a session, the client will discard any previous MQTT session information, stored on the MQTT broker,
and will start a new persistent session. When reconnecting, the existing MQTT session should be used. If
implemented by the broker, the persistent session must keep list of all subsctiption topics for that client, and
all the messages, published to these topics for the time, the client was offline. Any subscriptions, successfully
made during a session, will be automatically restored after reconnecting, even if the MQTT broker does not store
persistent session information. The configuration, stored in the client, applies when a session is started. That
means you have to end the current session and start a new one in order to effectively change a configuration
property (except for SetEventListener, which applies immediately). If any error occurs during the connection
attempt, it will be reported to the application through the `EventListener::OnError` callback.
    - `void EndSession()` - ends a session and disconnects the client (if a session was started).
    - `bool IsSessionStarted()` - returns `true` if a session was started.
    - `bool IsConnected()` - returns `true` if the client is actually connected to the TLS MQTT broker.
    - `bool Subscribe(const String& topic)` - subscribes to an MQTT topic. The function will first try to (re)connect
if not currently connected. Returns `true` if the subscribe command is successful. Else, any errors will be reported
through `EventListener::OnError` callback.
    - `bool Unsubscribe(const String& topic)` - unsubscribes from an MQTT topic. The function will first try to
(re)connect if not currently connected. Returns `true` if the unsubscribe command is successful. Else, any errors
will be reported through `EventListener::OnError` callback.
    - `bool Publish(const String& topic, const String& payload)` - publishes a message to an MQTT topic. The function
will first try to (re)connect if not currently connected. Returns `true` if the publish is successful. Else, any
errors will be reported through `EventListener::OnError` callback. Payload size is currently limited by the
implementation (paho mqtt embedded library `MAX_MQTT_PACKET_SIZE = 1024`).
    - `bool ListenForPrivateMessages()` - subscribes to a private message topic in order to receive private messages.
The private messages topic name is formed as `<hex encoded MQTT client id>/pm`. If `sokRecvKey` is set in `Identity`,
encrypted private messages can be received on this topic. Returns `true` if the subscribe command is successful.
Else, any errors will be reported through `EventListener::OnError` callback.
    - `bool SendPrivateMessage(const String& userIdTo, const String& payload, bool encrypt = true)` - publishes a
message on userIdTo's private messages topic (`<hex encoded userIdTo>/pm`). If `encrypt` is true and `sokSendKey`
is set in `Identity`, the message is sent encrypted. Returns `true` if the publish is successful. Else, any
errors will be reported through `EventListener::OnError` callback. Payload size is currently limited by the
implementation (paho mqtt embedded library `MAX_MQTT_PACKET_SIZE = 1024`).
    - `bool RunMessageLoop(unsigned long timeout)` - this function is supposed to be periodically invoked by the
client application. It will block for maximum of `timeout` milliseconds. It first checks the connection and tries
to reestablish it if lost. If the connection cound not be reestablished, or a session is not started, this function
will just sleep for `timeout` milliseconds. Else, it will invoke the MQTT message loop of the underlying MQTT library.
This loop is required to send/receive MQTT messages and to maintain the connection alive. Any errors when trying to
reestablish the connection or caused by connectivity issues during the underlying MQTT library loop will be reported
through `EventListener::OnError` callback.

A complete example usage of the library and a test client can be found in the `tests\iot_client` directory.
