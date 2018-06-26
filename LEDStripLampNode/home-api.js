const mqtt = require('mqtt')
const fs = require('fs')

var led = [
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0 ]

var settings = [0, 0]	

var MQTT_COLOR_TOPIC = '/home/ledlamp/color'
var MQTT_SETTINGS_TOPIC = '/home/ledlamp/settings'
var MQTT_SRV = 'raspi'

var mqttClient = mqtt.connect(MQTT_SRV)

fs.readFile('../LEDStripLampArduino/connection_conf.h', 'utf8', function (err, data) {
    if (err) {
        return console.log(err);
    }
    console.log(data);
    //mqtt.connect(MQTT_SRV)
});


mqttClient.on('connect', () => {
	client.subscribe(MQTT_COLOR_TOPIC)
	client.subscribe(MQTT_SETTINGS_TOPIC)
})

mqttClient.on('message', (topic, message) => {
	if (topic === MQTT_COLOR_TOPIC) {
		led[0] = parseMsgToArray(message.toString());
	} else if (topic === MQTT_SETTINGS_TOPIC) {
		led[1] = parseMsgToArray(message.toString());
	}
})

function publishColor () {
    mqttClient.publish(MQTT_SRV, parseArrayToMsg(led))
}

app.listen(3003);
console.log('LED-control-server running on port 3003');
