cd $(dirname "$0")
../../output/build/default/iot_client authServerUrl=https://iot.dev.miracl.net awsIoTCompliance=true identityFile=DEVICE_ID_2.identity mqttTlsBrokerAddr=mqtt.dev.miracl.net subscribeToTopic=mpin/test listenForPms=true