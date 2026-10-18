# HTTP Server & Custom CDN Setup Documentation

This project provides a lightweight HTTP/HTTPS server for Growtopia clients. It handles **login/server_data** responses and serves cached game assets.

## 1. Server Overview & Configuration

The HTTP server primarily handles two main tasks:

1. **`server_data.php` Endpoint:** Validates client details (version, platform, protocol), encrypts payload into a tokenized `meta` parameter, and returns connection parameters to the client (not used for now).

2. **`/cache/` CDN Endpoint:** Serves static game assets (such as `.rttex`, `.ogg`, `.mp3`, `.xml`, etc.) with built-in path traversal protection.

### Constants Configuration

The core configuration constants inside `main.go`:

```go
const SERVER_IP = "YOUR_SERVER_IP"
const LOGIN_URL = "g-topia-login.vercel.app"
const MAINTENANCE_MESSAGE = "`#Maintenance"
```

* **`SERVER_IP`**: The game server IP address (`ENet` connection target) sent to clients.
  * **Note:** If set to a local or invalid IP (e.g., `127.0.0.1`, `10.x.x.x`), the HTTP server binds directly to that specific IP. If set to a public/WAN IP, it binds to all network interfaces (`0.0.0.0`). **Cannot be left empty.**

* **`LOGIN_URL`**: The web-based login URL used by newer client versions.

* **`MAINTENANCE_MESSAGE`**: The message displayed on the client screen when the server is in maintenance mode.

---

## 2. Custom CDN (Cache) Setup Guide

Follow these steps to set up a custom CDN so clients can download game assets directly from your HTTP server:

### Step 1: Copy Cache Assets to `static/`

Locate the following folders from the Growtopia cache and move them into the `static/` directory at the root of your HTTP server:

* `game/`
* `interface/`
* `audio/`
* `GameData/`

> **Directory Structure Example:**
> ```text
> .
> ├── main.go
> └── static/
>     ├── game/
>     ├── interface/
>     ├── audio/
>     └── GameData/
> ```

### Step 2: Generate `filehashes.txt`

1. Run your setup script.
2. Select option **`[6] Generate filehashes.txt`**.
3. Provide the path to your `static/` folder when prompted.
4. Move the newly generated `filehashes.txt` file into the **`Runtime/`** directory.

### Step 3: Configure `Runtime/config.txt`

Open `Runtime/config.txt` and update the `cdn_server` field according to your deployment environment:

* **For VPS / Production:**
  ```text
  cdn_server|WAN_SERVER_IP|cache/|
  ```
  *(Replace `WAN_SERVER_IP` with your server's public IP address)*

* **For Localhost / Testing:**
  ```text
  cdn_server|127.0.0.1|cache/|
  ```