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

  // Apply saved states to switches
  document.getElementById("lamp-switch").checked = states.lamp;
  document.getElementById("pump-switch").checked = states.pump;

  return states;
}

// Save control state to localStorage
function saveControlState(control, state) {
  localStorage.setItem(`${control}-state`, state ? "1" : "0");
}

// Send heartbeat ping
function sendHeartbeat() {
  const payload = {
    action: "ping",
  };

  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(payload));
    console.log("Heartbeat dikirim");
  }
}

// Connect to WebSocket server
function connectWebSocket() {
  try {
    ws = new WebSocket("ws://" + window.location.host + "/aqua");

    ws.onopen = function () {
      console.log("WebSocket terhubung");
      updateConnectionStatus(true);

      // Clear reconnect interval if it exists
      if (reconnectInterval) {
        clearInterval(reconnectInterval);
        reconnectInterval = null;
      }

      if (heartbeatInterval) {
        clearInterval(heartbeatInterval);
      }
      heartbeatInterval = setInterval(sendHeartbeat, 3000);
    };

    ws.onmessage = function (event) {
      try {
        const payload = JSON.parse(event.data);
        handleMonitoringData(payload);
      } catch (error) {
        console.error("Kesalahan parsing pesan WebSocket:", error);
      }
    };

    ws.onerror = function (error) {
      console.error("Kesalahan WebSocket:", error);
      updateConnectionStatus(false);
    };

    ws.onclose = function () {
      console.log("WebSocket terputus");
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
    console.error("Gagal terhubung ke WebSocket:", error);
    updateConnectionStatus(false);

    // Attempt to reconnect every 5 seconds
    if (!reconnectInterval) {
      reconnectInterval = setInterval(connectWebSocket, 5000);
    }
  }
}

// Handle incoming monitoring data
function handleMonitoringData(payload) {
  if (!payload.monitor || payload.data === undefined) {
    console.warn("Payload pemantauan tidak valid:", payload);
    return;
  }

  const now = new Date().toLocaleTimeString();

  switch (payload.monitor) {
    case "water_temp":
      const tempElement = document.getElementById("temperature");
      const tempTimeElement = document.getElementById("temp-time");
      if (tempElement && tempTimeElement) {
        tempElement.textContent = payload.data;
        tempTimeElement.textContent = now;
        updateTemperatureStatus(payload.data);
      }
      break;

    case "water_level":
      const levelElement = document.getElementById("water-level");
      const levelTimeElement = document.getElementById("level-time");
      if (levelElement && levelTimeElement) {
        levelElement.textContent = payload.data;
        levelTimeElement.textContent = now;
        updateWaterLevelStatus(payload.data);
      }
      break;

    default:
      console.warn("Tipe monitor tidak dikenal:", payload.monitor);
  }
}

// Update water temperature status
function updateTemperatureStatus(value) {
  const tempElement = document.getElementById("temperature");
  if (!tempElement) return;

  const card = tempElement.closest(".card");
  if (!card) return;

  const statusBadge = card.querySelector(".card-status");
  if (!statusBadge) return;

  if (value < 25) {
    statusBadge.className = "card-status status-warning";
    statusBadge.textContent = "Terlalu Dingin";
  } else if (value >= 26 && value <= 29) {
    statusBadge.className = "card-status status-good";
    statusBadge.textContent = "Normal";
  } else if (value >= 30 && value <= 33) {
    statusBadge.className = "card-status status-warning";
    statusBadge.textContent = "Peringatan";
  } else if (value >= 34) {
    statusBadge.className = "card-status status-danger";
    statusBadge.textContent = "Bahaya!";
  } else {
    statusBadge.className = "card-status status-good";
    statusBadge.textContent = "Normal";
  }
}

// Update water level status
function updateWaterLevelStatus(value) {
  const levelElement = document.getElementById("water-level");
  if (!levelElement) return;

  const card = levelElement.closest(".card");
  if (!card) return;

  const statusBadge = card.querySelector(".card-status");
  if (!statusBadge) return;

  if (value >= 85) {
    statusBadge.className = "card-status status-good";
    statusBadge.textContent = "Normal";
  } else if (value >= 50 && value < 85) {
    statusBadge.className = "card-status status-warning";
    statusBadge.textContent = "Peringatan";
  } else {
    statusBadge.className = "card-status status-danger";
    statusBadge.textContent = "Kritis";
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
    console.log("Perintah kontrol dikirim:", payload);
  } else {
    console.warn("WebSocket tidak terhubung. Perintah tidak dikirim:", payload);
  }

  // Save state to localStorage
  saveControlState(control, state);
}

// Update connection status indicator
function updateConnectionStatus(connected) {
  const statusElement = document.getElementById("connection-status");
  const statusText = statusElement.querySelector(".status-text");

  if (connected) {
    statusElement.classList.add("connected");
    statusText.textContent = "Terhubung";
  } else {
    statusElement.classList.remove("connected");
    statusText.textContent = "Terputus";
  }
}

// Update timestamps for all sensors
function updateTimestamps() {
  const now = new Date().toLocaleTimeString();
  document.querySelectorAll(".timestamp span").forEach((span) => {
    if (span.textContent === "--:--:--") {
      span.textContent = now;
    }
  });
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

      console.log(`${controlName} ${state ? "NYALA" : "MATI"}`);
      sendControlCommand(controlName, state);
    });
  });

  // Connect to WebSocket
  connectWebSocket();

  // Update timestamps
  updateTimestamps();
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
