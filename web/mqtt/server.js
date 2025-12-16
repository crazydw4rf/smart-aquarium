import express from 'express';
import mqtt from 'mqtt';
import { WebSocketServer } from 'ws';
import { fileURLToPath } from 'url';
import { dirname } from 'path';
import dotenv from 'dotenv';

dotenv.config();

const app = express();
const PORT = process.env.PORT || 3000;

const MQTT_BROKER = process.env.MQTT_BROKER || 'mqtt://broker.hivemq.com:1883';
const MQTT_USER = process.env.MQTT_USER || '';
const MQTT_PASSWORD = process.env.MQTT_PASSWORD || '';

const TOPIC_COMMAND = 'aquarium/command';
const TOPIC_SENSOR = 'aquarium/sensor';
const TOPIC_CONTROL = 'aquarium/control';

const mqttClient = mqtt.connect(MQTT_BROKER, {
  username: MQTT_USER,
  password: MQTT_PASSWORD,
  reconnectPeriod: 2000,
});

let latestSensorData = {
  temp: null,
  level: null,
  led: null,
  pump: null,
  timestamp: null,
};

mqttClient.on('connect', () => {
  console.log('[MQTT] Connected');
  mqttClient.subscribe([TOPIC_SENSOR, TOPIC_CONTROL]);
});

mqttClient.on('message', (topic, message) => {
  try {
    const payload = JSON.parse(message.toString());
    console.log('[MQTT] <', message.toString());

    if (topic === TOPIC_SENSOR && payload.type === 'sensor') {
      // Update sensor data and control states together
      latestSensorData = {
        temp: payload.temp,
        level: payload.level,
        led: payload.led,
        pump: payload.pump,
        timestamp: payload.timestamp,
      };
      
      // Broadcast to all connected WebSocket clients
      broadcastToClients({
        type: 'update',
        data: latestSensorData,
      });
    }
  } catch (error) {
    console.error('Error parsing MQTT message:', error);
  }
});

mqttClient.on('error', (error) => {
  console.error('[MQTT] Error:', error.message);
});

mqttClient.on('close', () => {
  console.log('[MQTT] Disconnected');
});

/** @type {Set<WebSocket>} */
const wsConnections = new Set();

// Broadcast to all WebSocket clients
function broadcastToClients(message) {
  const data = JSON.stringify(message);
  wsConnections.forEach((client) => {
    if (client.readyState === 1) { // WebSocket.OPEN
      client.send(data);
    }
  });
}

// Serve static files
app.use(express.static('public'));

const server = app.listen(PORT, () => {
  console.log(`Web server running on http://localhost:${PORT}`);
  console.log(`MQTT broker: ${MQTT_BROKER}`);
});

// WebSocket server
const wss = new WebSocketServer({ server, path: '/aqua' });

wss.on('connection', (ws) => {
  console.log('[WS] Client connected');
  wsConnections.add(ws);

  // Send current state to new client
  ws.send(JSON.stringify({
    type: 'init',
    data: latestSensorData,
  }));

  // Handle incoming messages
  ws.on('message', (data) => {
    try {
      const payload = JSON.parse(data.toString());
      handleClientMessage(payload);
    } catch (error) {
      console.error('Error handling WebSocket message:', error);
    }
  });

  ws.on('close', () => {
    wsConnections.delete(ws);
    console.log('[WS] Client disconnected');
  });

  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
    wsConnections.delete(ws);
  });
});

// Handle messages from web clients
function handleClientMessage(payload) {
  if (payload.action === 'heartbeat') {
    // Send heartbeat to ESP32 via MQTT
    const msg = JSON.stringify({ type: 'heartbeat' });
    mqttClient.publish(TOPIC_COMMAND, msg);
  } else if (payload.action === 'control') {
    // Send control command to ESP32 via MQTT
    const command = {
      type: 'control',
      device: payload.control, // 'lamp' or 'pump'
      state: payload.state ? 'on' : 'off',
    };
    
    // Map 'lamp' to 'led' for ESP32
    if (command.device === 'lamp') {
      command.device = 'led';
    }
    
    const msg = JSON.stringify(command);
    mqttClient.publish(TOPIC_COMMAND, msg);
    console.log('[MQTT] >', msg);
  }
}


