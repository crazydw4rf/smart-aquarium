import express from "express";
import { join } from "path";
import dotenv from "dotenv";

dotenv.config();

const app = express();
const PORT = process.env.PORT || 3000;

// ThingSpeak Configuration
const CHANNEL_ID = process.env.THINGSPEAK_CHANNEL_ID;
const WRITE_API_KEY = process.env.THINGSPEAK_WRITE_API_KEY;

const THINGSPEAK_API = "https://api.thingspeak.com";

if (!CHANNEL_ID || !WRITE_API_KEY) {
  console.error("Error: Missing ThingSpeak configuration in .env file");
  process.exit(1);
}

app.use(express.json());
app.use(express.static("public"));

// Get current relay states
app.get("/api/status", async (req, res) => {
  try {
    const url = `${THINGSPEAK_API}/channels/${CHANNEL_ID}/feeds/last.json`;
    const response = await fetch(url);
    const data = await response.json();

    if (data.entry_id) {
      const ledState = parseInt(data.field3) === 1;
      const pumpState = parseInt(data.field4) === 1;
      console.log(`[ThingSpeak] Status: LED=${ledState ? "ON" : "OFF"}, Pump=${pumpState ? "ON" : "OFF"}`);
      res.json({ ledState, pumpState });
    } else {
      res.status(404).json({ error: "No data available" });
    }
  } catch (error) {
    console.error("Error fetching status:", error);
    res.status(500).json({ error: "Failed to fetch status" });
  }
});

// Update relay control
// Body: {device: "led"|"pump", state: true|false}
app.post("/api/control", async (req, res) => {
  try {
    const { device, state } = req.body;
    
    if (!device || state === undefined) {
      return res.status(400).json({ error: "Missing device or state" });
    }

    const value = state ? 1 : 0;
    let field;

    if (device === "led") {
      field = 3;
    } else if (device === "pump") {
      field = 4;
    } else {
      return res.status(400).json({ error: "Invalid device. Use 'led' or 'pump'" });
    }

    const url = `${THINGSPEAK_API}/update.json`;
    const body = new URLSearchParams({
      api_key: WRITE_API_KEY,
      [`field${field}`]: value
    });

    const response = await fetch(url, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body
    });

    const result = await response.json();
    const success = result.entry_id > 0;
    const status = success ? "SUCCESS" : "FAILED";
    console.log(`[ThingSpeak] ${device.toUpperCase()} field${field}=${value} - ${status} (entry: ${result.entry_id || 0})`);

    if (success) {
      res.json({ success: true, entryId: result.entry_id });
    } else {
      res.status(400).json({ error: "Update failed or rate limited" });
    }
  } catch (error) {
    console.error("Error updating control:", error);
    res.status(500).json({ error: "Failed to update control" });
  }
});

app.listen(PORT, () => {
  console.log(`Smart Aquarium Control running on http://localhost:${PORT}`);
  console.log(`ThingSpeak Channel: ${CHANNEL_ID}`);
});
