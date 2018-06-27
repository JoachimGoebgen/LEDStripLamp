var http = require('http');
var mqtt = require('mqtt');
var fs = require('fs');
var bodyParser = require("body-parser");
var express = require('express');

var app = express();
app.use(express['static'](__dirname));
app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());

var settings = new Array(2);
var led = new Array(12);
var savedModes = new Array(0);

var CONF_FILE_PATH = '../LEDStripLampArduino/connection_conf.h';
var MQTT_COLOR_TOPIC;
var MQTT_SETTINGS_TOPIC;
var MQTT_SAVE_TOPIC;
var MQTT_SRV;
var mqttClient;

initConfig();

Array.prototype.clean = function(deleteValue) {
  for (var i = 0; i < this.length; i++) {
    if (this[i] == deleteValue) {
      this.splice(i, 1);
      i--;
    }
  }
  return this;
};

mqttClient.on('connect', () => {
	mqttClient.subscribe(MQTT_COLOR_TOPIC)
	mqttClient.subscribe(MQTT_SETTINGS_TOPIC)
	mqttClient.subscribe(MQTT_SAVE_TOPIC)
});

mqttClient.on('message', (topic, message) => {
	if (topic === MQTT_COLOR_TOPIC) {
		led = message.toString().split(" ");
	} else if (topic === MQTT_SETTINGS_TOPIC) {
		settings = message.toString().split(" ");
	} else if (topic === MQTT_SAVE_TOPIC) {
                saveModeStrToFile(message.toString());
		var newMode = parseModeFromString(message.toString());
		savedModes.push(newMode);
	}

	//console.log(message);
});

app.get('/savedModes', function (req, res) {
    res.status(200).send(savedModes);
});

app.post('/color', function (req, res) {
    	led = req.body['colArr[]'];
	mqttClient.publish(MQTT_COLOR_TOPIC, parseArrayToMsg(led));
	res.status(200).send(led);
});

app.post('/settings', function (req, res) {
    	settings = req.body['settArr[]'];
        mqttClient.publish(MQTT_SETTINGS_TOPIC, parseArrayToMsg(led));
    	res.status(200).send(settings);
});

function parseArrayToMsg(arr) {
	
};

function saveModeStrToFile(modeStr) {
	fs.appendFileSync(CONF_FILE_PATH, "\n" + modeStr);
};

function initConfig() {
    	var data = String(fs.readFileSync(CONF_FILE_PATH));

	MQTT_COLOR_TOPIC = data.substring(data.search("MQTT_COLOR_TOPIC")+16).split("\"")[1];
    	MQTT_SETTINGS_TOPIC = data.substring(data.search("MQTT_SETTINGS_TOPIC")+19).split("\"")[1];
    	MQTT_SAVE_TOPIC = MQTT_COLOR_TOPIC.concat("/save");
	var port = data.substring(data.search("MQTT_SRV_PORT")+13).split(/\s+/)[1];
    	MQTT_SRV = "mqtt://".concat(data.substring(data.search("MQTT_SRV_IP")+11).split("\"")[1]).concat(":").concat(port);

    	mqttClient = mqtt.connect(MQTT_SRV);
    	//console.log(MQTT_SRV);
    	//console.log(MQTT_COLOR_TOPIC);
    	//console.log(MQTT_SETTINGS_TOPIC);
	//console.log(MQTT_SAVE_TOPIC);

	var savedModesStrings = String(fs.readFileSync('../saved_modes')).split(/\n/);
	for (i = 0; i < savedModesStrings.length; i++) {
		var newMode = parseModeFromString(savedModesStrings[i]);
		savedModes.push(newMode);
	}

	console.log(savedModes);
};

function parseModeFromString(str) {
	var strSplit = str.split(",");
        var newMode = new Array(3);
        newMode[0] = strSplit[0];
        newMode[1] = cleanEmptyEntries(strSplit[1].split(" "));
	newMode[2] = cleanEmptyEntries(strSplit[2].split(" "));
	return newMode;
};

function cleanEmptyEntries(arr) {
  for (var i = 0; i < arr.length; i++) {
    if (arr[i] == "") {
      arr.splice(i, 1);
      i--;
    }
  }
  return arr;
};


app.listen(3003);
console.log('LED-control-server running on port 3003');
