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
var colors = new Array(12);
var presets = new Array(0);

var CONF_FILE_PATH = '../LEDStripLampArduino/connection_conf.h';
var PRESET_FILE_PATH = '../saved_modes';
var MQTT_COLOR_TOPIC;
var MQTT_SETTINGS_TOPIC;
var MQTT_SRV;
var mqttClient;

initConfig();

mqttClient.on('connect', () => {
	mqttClient.subscribe(MQTT_COLOR_TOPIC)
	mqttClient.subscribe(MQTT_SETTINGS_TOPIC)
});

mqttClient.on('message', (topic, message) => {
	if (topic === MQTT_COLOR_TOPIC) {
		led = message.toString().split(" ");
	} else if (topic === MQTT_SETTINGS_TOPIC) {
		settings = message.toString().split(" ");
	}

	//console.log(message);
});


app.get('/colors', function (req, res) {
        res.send(colors);
});

app.get('/settings', function (req, res) {
        res.send(settings);
});

app.get('/presets', function (req, res) {
    	res.send(presets);
});

app.post('/colors', function (req, res) {
        colors = req.body['colorArr[]'];
        mqttClient.publish(MQTT_COLOR_TOPIC, colors.join(" "));
        res.status(200).send(colors);
});

app.post('/settings', function (req, res) {
        settings = req.body['settingArr[]'];
        mqttClient.publish(MQTT_SETTINGS_TOPIC, settings.join(" "));
        res.status(200).send(settings);
});

app.post('/presets', function (req, res) {
        presets.push(req.body['presetArr[]']);
        savePresetsToFile();
        res.status(200).send(presets);
});


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

	// read presets from file
	var lines = cleanEmptyEntries(String(fs.readFileSync(PRESET_FILE_PATH)).split(/\n/));
	for (i = 0; i < lines.length; i++) {
		var newPreset = parsePresetFromString(lines[i]);
		presets.push(newPreset);
	}

	//console.log(presets);
};

function savePresetsToFile() {
        var lines = new Array(presets.length);
        for (i = 0; i < presets.length; i++) {
                var temp = new Array(3);
                temp[0] = presets[i][0];
                temp[1] = presets[i][1].join(" ");
                temp[2] = presets[i][2].join(" ");
                lines[i] = temp.join(", ");
        }

        fs.writeFile(PRESET_FILE_PATH, lines.join("\n"));
};

function parsePresetFromString(str) {
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
