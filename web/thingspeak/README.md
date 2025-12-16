# Smart Aquarium - ThingSpeak Web Control

Web interface minimalis untuk mengontrol relay LED dan Pompa menggunakan ThingSpeak API.

## Stack
- **Runtime**: Bun
- **Framework**: Express.js
- **Frontend**: Vanilla HTML/CSS/JS

## Setup

1. Edit konfigurasi di `server.js`:
```javascript
const CHANNEL_ID = "YOUR_CHANNEL_ID";
const WRITE_API_KEY = "YOUR_WRITE_API_KEY";
const READ_API_KEY = "YOUR_READ_API_KEY";
```

2. Install dependencies:
```bash
bun install
```

3. Run server:
```bash
bun start
# atau untuk development dengan auto-reload:
bun run dev
```

4. Buka browser di `http://localhost:3000`

## Fitur

- 📊 Real-time sensor monitoring (suhu & ketinggian air)
- 🎛️ Toggle control untuk LED & Pompa
- 🔄 Auto-refresh setiap 5 detik
- ⚡ Minimalist & responsive UI

## API Endpoints

- `GET /api/status` - Ambil data sensor dan status kontrol
- `POST /api/control/led` - Toggle LED (body: `{state: true/false}`)
- `POST /api/control/pump` - Toggle Pompa (body: `{state: true/false}`)

## Note

ThingSpeak memiliki rate limit 15 detik antar update. Pastikan tidak terlalu sering mengubah kontrol.
