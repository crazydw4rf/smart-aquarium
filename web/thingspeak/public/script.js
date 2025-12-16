let currentLedState = false;
let currentPumpState = false;

async function fetchInitialState() {
  try {
    const response = await fetch("/api/status");
    const data = await response.json();

    if (data.ledState !== undefined) {
      currentLedState = data.ledState;
      currentPumpState = data.pumpState;

      updateStatusUI("led", currentLedState);
      updateStatusUI("pump", currentPumpState);

      const ledBtn = document.getElementById("led-btn");
      const pumpBtn = document.getElementById("pump-btn");
      ledBtn.classList.toggle("active", currentLedState);
      pumpBtn.classList.toggle("active", currentPumpState);
    }
  } catch (error) {
    console.error("Error fetching initial state:", error);
  }
}

function showToast(message) {
  const toast = document.getElementById("toast");
  toast.textContent = message;
  toast.classList.add("show");
  setTimeout(() => {
    toast.classList.remove("show");
  }, 3000);
}

function updateStatusUI(device, isOn) {
  const statusEl = document.getElementById(`${device}-status`);
  statusEl.textContent = isOn ? "ON" : "OFF";
  statusEl.classList.toggle("on", isOn);
}

async function toggleLED() {
  const btn = document.getElementById("led-btn");
  btn.disabled = true;

  try {
    const newState = !currentLedState;
    const response = await fetch("/api/control", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ device: "led", state: newState }),
    });

    const data = await response.json();

    if (data.success) {
      currentLedState = newState;
      updateStatusUI("led", currentLedState);
      btn.classList.toggle("active", currentLedState);
      showToast("LED updated");
    } else {
      showToast("Update failed. Wait 15 seconds.");
    }
  } catch (error) {
    console.error("Error:", error);
  } finally {
    btn.disabled = false;
  }
}

async function togglePump() {
  const btn = document.getElementById("pump-btn");
  btn.disabled = true;

  try {
    const newState = !currentPumpState;
    const response = await fetch("/api/control", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ device: "pump", state: newState }),
    });

    const data = await response.json();

    if (data.success) {
      currentPumpState = newState;
      updateStatusUI("pump", currentPumpState);
      btn.classList.toggle("active", currentPumpState);
      showToast("Pump updated");
    } else {
      showToast("Update failed. Wait 15 seconds.");
    }
  } catch (error) {
    console.error("Error:", error);
  } finally {
    btn.disabled = false;
  }
}

// Fetch initial state on page load
fetchInitialState();
