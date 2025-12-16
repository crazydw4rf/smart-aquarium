// WebSocket connection
let ws = null;
let reconnectInterval = null;
let heartbeatInterval = null;

// Load saved control states from localStorage
function loadControlStates() {
  const states = {
    lamp: localStorage.getItem("lamp-state") === "1",
    pump: localStorage.getItem("pump-state") === "1",
  };

  // Apply saved states to switches and labels
  const lampSwitch = document.getElementById("lamp-switch");
  const lampStatus = document.getElementById("lamp-status");
  if (lampSwitch) lampSwitch.checked = states.lamp;
  if (lampStatus) {
    lampStatus.textContent = states.lamp ? "ON" : "OFF";
    lampStatus.classList.toggle("on", states.lamp);
  }

  const pumpSwitch = document.getElementById("pump-switch");
  const pumpStatus = document.getElementById("pump-status");
  if (pumpSwitch) pumpSwitch.checked = states.pump;
  if (pumpStatus) {
    pumpStatus.textContent = states.pump ? "ON" : "OFF";
    pumpStatus.classList.toggle("on", states.pump);
  }

  return states;
}

// Save control state to localStorage
function saveControlState(control, state) {
  localStorage.setItem(`${control}-state`, state ? "1" : "0");
}

// Send heartbeat to trigger sensor data
function sendHeartbeat() {
  const payload = {
    action: "heartbeat",
  };

  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(payload));
  }
}

// Connect to WebSocket server
function connectWebSocket() {
  try {
    ws = new WebSocket("ws://" + window.location.host + "/aqua");

    ws.onopen = function () {
      updateConnectionStatus(true);

      // Clear reconnect interval if it exists
      if (reconnectInterval) {
        clearInterval(reconnectInterval);
        reconnectInterval = null;
      }

      // Start heartbeat interval (every 3 seconds)
      if (heartbeatInterval) {
        clearInterval(heartbeatInterval);
      }
      heartbeatInterval = setInterval(sendHeartbeat, 3000);
      
      // Send initial heartbeat immediately
      setTimeout(sendHeartbeat, 100);
    };

    ws.onmessage = function (event) {
      try {
        const payload = JSON.parse(event.data);
        handleServerMessage(payload);
      } catch (error) {
        console.error("Error parsing WebSocket message:", error);
      }
    };

    ws.onerror = function (error) {
      console.error("WebSocket error:", error);
      updateConnectionStatus(false);
    };

    ws.onclose = function () {
      updateConnectionStatus(false);

      // Stop heartbeat
      if (heartbeatInterval) {
        clearInterval(heartbeatInterval);
        heartbeatInterval = null;
      }

      // Attempt to reconnect every 5 seconds
      if (!reconnectInterval) {
        reconnectInterval = setInterval(connectWebSocket, 5000);
      }
    };
  } catch (error) {
    console.error("Failed to connect to WebSocket:", error);
    updateConnectionStatus(false);

    // Attempt to reconnect every 5 seconds
    if (!reconnectInterval) {
      reconnectInterval = setInterval(connectWebSocket, 5000);
    }
  }
}

// Handle incoming messages from server
function handleServerMessage(payload) {
  if (payload.type === 'init' || payload.type === 'update') {
    // Update all data (sensor + control states)
    if (payload.data) {
      updateSensorData(payload.data);
      updateControlStates(payload.data);
    }
  }
}

// Update sensor displays
function updateSensorData(data) {
  if (!data) return;

  if (data.temp !== null && data.temp !== undefined) {
    const tempElement = document.getElementById("temperature");
    if (tempElement) {
      tempElement.textContent = data.temp.toFixed(1);
    }
  }

  if (data.level !== null && data.level !== undefined) {
    const levelElement = document.getElementById("water-level");
    if (levelElement) {
      levelElement.textContent = data.level.toFixed(1);
    }
  }
}

// Update control states based on data from ESP32
function updateControlStates(data) {
  if (!data) return;

  // Update LED/Lamp switch and status
  if (data.led !== null && data.led !== undefined) {
    const lampSwitch = document.getElementById("lamp-switch");
    const lampStatus = document.getElementById("lamp-status");
    const isOn = data.led === 'on';
    
    if (lampSwitch) {
      lampSwitch.checked = isOn;
      saveControlState('lamp', isOn);
    }
    
    if (lampStatus) {
      lampStatus.textContent = isOn ? "ON" : "OFF";
      lampStatus.classList.toggle("on", isOn);
    }
  }

  // Update Pump switch and status
  if (data.pump !== null && data.pump !== undefined) {
    const pumpSwitch = document.getElementById("pump-switch");
    const pumpStatus = document.getElementById("pump-status");
    const isOn = data.pump === 'on';
    
    if (pumpSwitch) {
      pumpSwitch.checked = isOn;
      saveControlState('pump', isOn);
    }
    
    if (pumpStatus) {
      pumpStatus.textContent = isOn ? "ON" : "OFF";
      pumpStatus.classList.toggle("on", isOn);
    }
  }
}



// Send control command via WebSocket
function sendControlCommand(control, state) {
  const payload = {
    action: "control",
    control: control,
    state: state ? 1 : 0,
  };

  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(payload));
  }

  // Save state to localStorage
  saveControlState(control, state);
}

// Update connection status indicator
function updateConnectionStatus(connected) {
  const statusElement = document.getElementById("connection-status");
  
  if (connected) {
    statusElement.classList.add("connected");
    statusElement.textContent = "Terhubung";
  } else {
    statusElement.classList.remove("connected");
    statusElement.textContent = "Terputus";
  }
}

// Map switch IDs to control names
const controlMap = {
  "lamp-switch": "lamp",
  "pump-switch": "pump",
};

// Initialize on page load
window.addEventListener("DOMContentLoaded", function () {
  // Load saved control states
  loadControlStates();

  // Control switches event listeners
  document.querySelectorAll(".switch input").forEach((toggle) => {
    toggle.addEventListener("change", function () {
      const controlName = controlMap[this.id];
      const state = this.checked;
      
      // Update label immediately for responsive UI
      const statusId = controlName === "lamp" ? "lamp-status" : "pump-status";
      const statusElement = document.getElementById(statusId);
      if (statusElement) {
        statusElement.textContent = state ? "ON" : "OFF";
        statusElement.classList.toggle("on", state);
      }
      
      sendControlCommand(controlName, state);
    });
  });

  // Connect to WebSocket
  connectWebSocket();
});

// Clean up on page unload
window.addEventListener("beforeunload", function () {
  if (heartbeatInterval) {
    clearInterval(heartbeatInterval);
  }
  if (reconnectInterval) {
    clearInterval(reconnectInterval);
  }
  if (ws) {
    ws.close();
  }
});
