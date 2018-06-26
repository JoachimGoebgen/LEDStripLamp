var http = require('http');
var mqtt = require('mqtt');
var fs = require('fs');
var express = require('express');
var app = express();
app.use(express['static'](__dirname));

var led = [
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0 ];

var settings = [0, 0];

var MQTT_COLOR_TOPIC;
var MQTT_SETTINGS_TOPIC;
var MQTT_SRV;
var mqttClient;

initConfig();

function initConfig() {
    var data = String(fs.readFileSync('../LEDStripLampArduino/connection_conf.h'));

    MQTT_COLOR_TOPIC = data.substring(data.search("MQTT_COLOR_TOPIC")+16).split("\"")[1];
    MQTT_SETTINGS_TOPIC = data.substring(data.search("MQTT_SETTINGS_TOPIC")+19).split("\"")[1];
    var port = data.substring(data.search("MQTT_SRV_PORT")+13).split(/\s+/)[1];
    MQTT_SRV = "mqtt://".concat(data.substring(data.search("MQTT_SRV_IP")+11).split("\"")[1]).concat(":").concat(port);

    mqttClient = mqtt.connect(MQTT_SRV);

    console.log(MQTT_SRV);
    console.log(MQTT_COLOR_TOPIC);
    console.log(MQTT_SETTINGS_TOPIC);

}


//mqttClient.on('connect', () => {
//	mqttClient.subscribe(MQTT_COLOR_TOPIC)
//	mqttClient.subscribe(MQTT_SETTINGS_TOPIC)
//});

//mqttClient.on('message', (topic, message) => {
//	if (topic === MQTT_COLOR_TOPIC) {
//		led[0] = parseMsgToArray(message.toString());
//	} else if (topic === MQTT_SETTINGS_TOPIC) {
//		led[1] = parseMsgToArray(message.toString());
//	}
//});

function publishColor () {
    mqttClient.publish(MQTT_SRV, parseArrayToMsg(led))
};

app.listen(3003);
console.log('LED-control-server running on port 3003');
