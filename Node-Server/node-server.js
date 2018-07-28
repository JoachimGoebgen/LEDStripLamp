var mqtt = require('mqtt');
var fs = require('fs');

var settings = new Array(2);
var colors = new Array(12);
var presets = new Array(0);

var CONF_FILE_PATH = '../connection_conf.h';
var PRESET_FILE_PATH = '../presets';
var MQTT_COLOR_TOPIC;
var MQTT_SETTINGS_TOPIC;
var MQTT_BRIGHTNESS_TOPIC;
var MQTT_PRESET_TOPIC;
var MQTT_SRV;
var mqttClient;

var numSides = 4;
var brightnessStepPerc = 0.1;

initConfig();

mqttClient.on('connect', () => {
	mqttClient.subscribe(MQTT_COLOR_TOPIC)
	mqttClient.subscribe(MQTT_SETTINGS_TOPIC)
	mqttClient.subscribe(MQTT_BRIGHTNESS_TOPIC)
	mqttClient.subscribe(MQTT_PRESET_TOPIC)
	
	for (i = 0; i <= numSides; i++) {
		// connect to f.e. "/home/lamp/color0", "/home/lamp/color1", ... 
		// where 0 is broadcast-like for all sides and [1, 2, ..., numSides] represent the lamps' sides
		mqttClient.subscribe(MQTT_COLOR_TOPIC.concat(i));
	}
});

mqttClient.on('message', (topic, message) => {
	var msgStr = message.toString();
	
	// normal color update 
	if (topic === MQTT_COLOR_TOPIC) { 
		colors = msgStr.split(" ");
		
	// mode and speed update
	} else if (topic === MQTT_SETTINGS_TOPIC) { 
		settings = msgStr.split(" ");
	
	// update a single side and re-publish with whole colors-array
	} else if (topic.includes(MQTT_COLOR_TOPIC)) { 
		var rgb;
		if (msgStr.startsWith("#")) { 
			rgb = hexToRgb(msgStr); // convert rgb from hex, f.e. "#f34ff4"
		} 
		else { 
			rgb = cleanEmptyEntries(msgStr.split(" ")); // parse rgb from string, f.e. "255 40 0"
		} 
		
		var side = topic.substring(topic.length - 1, topic.length); //
		
		if (side === 0) { // side==0 is broadcast-like to set all sides at once with just one submitted color
			for (i = 0; i < numSides; i++) {
				colors[i*3] = rgb[0];
				colors[i*3+1] = rgb[1];
				colors[i*3+2] = rgb[2];
			}
		} else { // otherwise, set the specified side, side is in range {1, ..., numSides}
			colors[(side-1)*3] = rgb[0];
			colors[(side-1)*3+1] = rgb[1];
			colors[(side-1)*3+2] = rgb[2];
		}
		
		mqttClient.publish(MQTT_COLOR_TOPIC, colors.join(" "));
	
	// increase or decrease brightness
	} else if (topic === MQTT_BRIGHTNESS_TOPIC) { 
		var sign; // cast '+' to 1 and '-' to -1
		if (msgStr[0] === '+') {
			sign = 1;
		} else if (msgStr[0] === '-'){
			sign = -1;
		} else {
			return;
		}
		
		for (i = 0; i < numSides; i++) {
			var newR = colors[i*3] * (1 + sign * brightnessStepPerc);
			var newG = colors[i*3+1] * (1 + sign * brightnessStepPerc);
			var newB = colors[i*3+2] * (1 + sign * brightnessStepPerc);
			// only update brightness if colors do not exceed the bounds
			if ((sign > 0 && newR <= 255 && newG <= 255 && newB <= 255)
				|| (sign < 0 && newR >= 0 && newG >= 0 && newB >= 0))
			{
				colors[i*3] = newR;
				colors[i*3+1] = newG;
				colors[i*3+2] = newB;
			}
		}
		
		mqttClient.publish(MQTT_COLOR_TOPIC, colors.join(" "));
	
	// either load preset or save current state as preset
	} else if (topic === MQTT_PRESET_TOPIC) { 
		var strSplit = cleanEmptyEntries(msgStr.split(" "));
		if (strSplit.length > 1 && strSplit[0] === "save") { // save
			var id = strSplit[1];
			var name; // optional
			if (strSplit.length == 3) { name = strSplit[2]; } else { name = ""; }
			var newPreset = createNewPreset(id, name);
			var idExists = false;
			for (i = 0; i < presets.length; i++) {
				if (presets[i][0][0] === id) {
					presets[i] = newPreset;
					idExists = true;
				}
			}
			if (idExists == false) {
				presets.push(createNewPreset(id, name));
			}
			savePresetsToFile();
			
		} else if (strSplit.length == 1) { // load
			for (i = 0; i < presets.length; i++) {
				if (presets[i][0][0] === strSplit[0] || presets[i][0][1] === strSplit[0]) { // if mqtt-string equals either id or name, publish the saved preset
					mqttClient.publish(MQTT_COLOR_TOPIC, presets[i][1].join(" "));
					mqttClient.publish(MQTT_SETTINGS_TOPIC, presets[i][2].join(" "));
				}
			}
		}
	}
	
	console.log(msgStr);
});

function initConfig() {
	var data = String(fs.readFileSync(CONF_FILE_PATH));

	MQTT_COLOR_TOPIC = data.substring(data.search("MQTT_COLOR_TOPIC")+16).split("\"")[1];
	MQTT_SETTINGS_TOPIC = data.substring(data.search("MQTT_SETTINGS_TOPIC")+19).split("\"")[1];
	MQTT_BRIGHTNESS_TOPIC = data.substring(data.search("MQTT_BRIGHTNESS_TOPIC")+21).split("\"")[1];
	MQTT_PRESET_TOPIC = data.substring(data.search("MQTT_PRESET_TOPIC")+17).split("\"")[1];
	var port = data.substring(data.search("MQTT_SRV_PORT")+13).split(/\s+/)[1];
	MQTT_SRV = "mqtt://".concat(data.substring(data.search("MQTT_SRV_IP")+11).split("\"")[1]).concat(":").concat(port);

	mqttClient = mqtt.connect(MQTT_SRV);

	// read presets from file
	var lines = cleanEmptyEntries(String(fs.readFileSync(PRESET_FILE_PATH)).split(/\n/));
	for (i = 0; i < lines.length; i++) {
		var newPreset = parsePresetFromString(lines[i]);
		presets.push(newPreset);
	}

	console.log(presets);
};

function createNewPreset(id, name) {
	var newPreset = new Array(3);
	newPreset[0] = [id, name];
	newPreset[1] = colors.slice();
	newPreset[2] = settings.slice();
	return newPreset;
}

// format: ID name, r1 g1 b1 r2 g2 b2 r3 g3 b3 r4 g4 b4, mode speed
function savePresetsToFile() {
	var lines = new Array(presets.length);
	for (i = 0; i < presets.length; i++) {
		var temp = new Array(3);
		temp[0] = presets[i][0].join(" ");
		temp[1] = presets[i][1].join(" ");
		temp[2] = presets[i][2].join(" ");
		lines[i] = temp.join(", ");
	}

	fs.writeFile(PRESET_FILE_PATH, lines.join("\n"));
};

function parsePresetFromString(str) {
	var strSplit = str.split(",");
	var newPreset = new Array(3);
	newPreset[0] = cleanEmptyEntries(strSplit[0].split(" "));
	newPreset[1] = cleanEmptyEntries(strSplit[1].split(" "));
	newPreset[2] = cleanEmptyEntries(strSplit[2].split(" "));
	return newPreset;
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

// see https://stackoverflow.com/questions/5623838/rgb-to-hex-and-hex-to-rgb
function hexToRgb(hex) {
	// Expand shorthand form (e.g. "03F") to full form (e.g. "0033FF")
	var shorthandRegex = /^#?([a-f\d])([a-f\d])([a-f\d])$/i;
	hex = hex.replace(shorthandRegex, function(m, r, g, b) {
		return r + r + g + g + b + b;
	});
	
	var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
	return result ? new Array(
		parseInt(result[1], 16), 
		parseInt(result[2], 16), 
		parseInt(result[3], 16)
	) : null;
}

