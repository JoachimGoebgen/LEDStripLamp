var mqtt = require('mqtt');
var fs = require('fs');
var path = require('fs');

var settings = new Array(2);
var colors = new Array(12);
var presets = new Array(0);

var CONF_FILE_PATH = path.resolve(__dirname, 'connection_conf.h');
var PRESET_FILE_PATH = path.resolve(__dirname, 'presets');
var MQTT_COLOR_TOPIC;
var MQTT_COLOR_TOPIC_BROADCAST;
var MQTT_MODE_TOPIC;
var MQTT_BRIGHTNESS_TOPIC;
var MQTT_LOADPRESET_TOPIC;
var MQTT_SAVEPRESET_TOPIC;
var MQTT_SRV;
var mqttClient;

var numSides = 4;
var brightnessStepPerc = 0.1;

initConfig();

mqttClient.on('connect', () => {
	// connect to all sub-topics of ~/color/...
	// where 0 is broadcast-like for all sides and [1, 2, ..., numSides] represent the lamps' sides
	mqttClient.subscribe(MQTT_COLOR_TOPIC);
	mqttClient.subscribe(MQTT_COLOR_TOPIC.concat("/+"));
	mqttClient.subscribe(MQTT_MODE_TOPIC);
	mqttClient.subscribe(MQTT_BRIGHTNESS_TOPIC);
	mqttClient.subscribe(MQTT_LOADPRESET_TOPIC);
	mqttClient.subscribe(MQTT_SAVEPRESET_TOPIC);
});

mqttClient.on('message', (topic, message) => {
	var msgStr = message.toString();
	
	// mode and speed update
	if (topic === MQTT_MODE_TOPIC) { 
		settings = msgStr.split(" ");
	
	// color-parent-topic: Contains color-values for each side
	} else if (topic === MQTT_COLOR_TOPIC) {
		if (msgStr.startsWith("#")) { 
			colors = hexToRgb(msgStr); // convert rgb from hex, f.e. "#f34ff4"
		} 
		else { 
			colors = cleanEmptyEntries(msgStr.split(" ")); // parse rgb from string, f.e. "255 40 0"
		} 
				
	// color-sub-topic: To update a single side or all sides with the same color at once 	
	} else if (topic.includes(MQTT_COLOR_TOPIC)) { 
		var rgb;
		if (msgStr.startsWith("#")) { 
			rgb = hexToRgb(msgStr); // convert rgb from hex, f.e. "#f34ff4"
		} 
		else { 
			rgb = cleanEmptyEntries(msgStr.split(" ")); // parse rgb from string, f.e. "255 40 0"
		} 
		
		var side = topic.substring(topic.length - 1, topic.length); 
		
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
			var brightnessSum = colors[i*3] + colors[i*3+1] + colors[i*3+2];
			var ratioR = colors[i*3] / brightnessSum;
			var ratioG = colors[i*3+1] / brightnessSum;
			var ratioB = colors[i*3+2] / brightnessSum;
			brightnessSum = brightnessSum * (1 + sign * brightnessStepPerc);
			var newR = ratioR * brightnessSum;
			var newG = ratioG * brightnessSum;
			var newB = ratioB * brightnessSum;
			
			// only update brightness if colors do not exceed the bounds
			if (sign > 0 && newR <= 255 && newG <= 255 && newB <= 255)
			{
				colors[i*3] = Math.ceil(newR);
				colors[i*3+1] = Math.ceil(newG);
				colors[i*3+2] = Math.ceil(newB);
			}
			else if (sign < 0 && newR >= 0 && newG >= 0 && newB >= 0)
			{
				colors[i*3] = Math.floor(newR);
				colors[i*3+1] = Math.floor(newG);
				colors[i*3+2] = Math.floor(newB);
			}
		}
		
		mqttClient.publish(MQTT_COLOR_TOPIC_BROADCAST, colors.join(" "));
	
	// either load preset or save current state as preset
	} else if (topic === MQTT_SAVEPRESET_TOPIC) { 
		var strSplit = cleanEmptyEntries(msgStr.split(" "));
		if (strSplit.length >= 1) { // save
			var id = strSplit[0];
			var name; // optional
			if (strSplit.length == 2) { name = strSplit[1]; } else { name = ""; }
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
		}
	} else if (topic === MQTT_LOADPRESET_TOPIC) {
		var strSplit = cleanEmptyEntries(msgStr.split(" "));
		if (strSplit.length == 1) { // load
			for (i = 0; i < presets.length; i++) {
				if (presets[i][0][0] === strSplit[0] || presets[i][0][1] === strSplit[0]) { // if mqtt-string equals either id or name, publish the saved preset
					mqttClient.publish(MQTT_COLOR_TOPIC_BROADCAST, presets[i][1].join(" "));
					mqttClient.publish(MQTT_MODE_TOPIC, presets[i][2].join(" "));
				}
			}
		}
	}
	
	console.log("msgStr: ".concat(msgStr));
	console.log("colors: ".concat(colors));
});

function initConfig() {
	var data = String(fs.readFileSync(CONF_FILE_PATH));

	MQTT_COLOR_TOPIC = data.substring(data.search("MQTT_COLOR_TOPIC")+16).split("\"")[1];
	MQTT_COLOR_TOPIC_BROADCAST = MQTT_COLOR_TOPIC.concat("/0");
	MQTT_MODE_TOPIC = data.substring(data.search("MQTT_MODE_TOPIC")+15).split("\"")[1];
	MQTT_BRIGHTNESS_TOPIC = data.substring(data.search("MQTT_BRIGHTNESS_TOPIC")+21).split("\"")[1];
	MQTT_LOADPRESET_TOPIC = data.substring(data.search("MQTT_LOADPRESET_TOPIC")+21).split("\"")[1];
	MQTT_SAVEPRESET_TOPIC = data.substring(data.search("MQTT_SAVEPRESET_TOPIC")+21).split("\"")[1];
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

