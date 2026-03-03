# wxdash

Real-time weather dashboard for Raspberry Pi kiosk display.

Displays live conditions from a Davis Instruments WeatherLink Live over HTTP and UDP, with optional air quality data from a PurpleAir sensor. Designed to run fullscreen, unattended, and always-on.

## Features

- Real-time data from WeatherLink Live (HTTP polling + UDP broadcast)
- Arc gauges for temperature, humidity, pressure, wind speed, UV index, solar radiation, rain rate, dew point, and feels-like
- Wind rose histogram showing directional frequency over a rolling window
- AQI multi-ring gauge from a local PurpleAir sensor
- Sparkline trend graphs for all sensors
- Full-screen kiosk mode (`--kiosk` flag)
- Responsive layout (720p to 4K)

## Requirements

- Qt 6 (Core, Network, Quick, Qml, Svg, Test)
- CMake 3.22 or later
- Ninja (recommended)
- C++17 compiler
- Davis Instruments WeatherLink Live reachable on local network
- PurpleAir sensor on local network (optional, for AQI gauge)

## Building

Standard desktop build:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/src/wxdash           # windowed mode
./build/src/wxdash --kiosk   # fullscreen mode
```

## Raspberry Pi Deployment

This section documents how to deploy wxdash as a kiosk on a Raspberry Pi 4 or 5.

### Prerequisites

- Raspberry Pi 4 or 5 running Pi OS Bookworm (64-bit recommended)
- Qt 6 development packages:

```bash
sudo apt install qt6-base-dev qt6-declarative-dev qt6-svg-dev qt6-shadertools-dev \
    libqt6quick6 libqt6qml6 libxkbcommon-dev cmake ninja-build
```

- Mesa V3D driver — this is the default on Pi OS Bookworm; no additional configuration is needed.
- GPU memory: the default 76 MB is sufficient for Mesa. Increase to 128 MB only if you see rendering issues by adding `gpu_mem=128` to `/boot/firmware/config.txt` and rebooting.

### Build and Install

Clone and build on the Pi (or cross-compile and copy):

```bash
git clone <repo-url> wxdash
cd wxdash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
```

**Pi Zero 2 W note:** The Pi Zero 2 W only has 512 MB of RAM, which is not enough to compile Qt 6 C++ without swap. Add a swap file and limit build parallelism:

```bash
sudo fallocate -l 1G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
cmake --build build -j1
```

To make the swap permanent, add this line to `/etc/fstab`:

```
/swapfile none swap sw 0 0
```

The build will be slow (~30-60 min) but should complete without the OOM killer rebooting the Pi.

Install the kiosk component (binary, QML module, systemd service, EGLFS config, desktop entry, icon):

```bash
sudo cmake --install build --component kiosk
```

Alternatively, use the convenience target that also enables the systemd service:

```bash
sudo cmake --build build --target install-kiosk
```

### User Permissions

The systemd service runs as the user who configured the build (the value of `SERVICE_USER`, which defaults to `$USER` at `cmake` configure time). That user must be in the `render`, `video`, and `input` groups for DRM and input device access:

```bash
sudo usermod -aG render,video,input $USER
```

Log out and back in (or reboot) for the group changes to take effect.

To override the service user explicitly, pass `-DSERVICE_USER=<username>` when running CMake:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DSERVICE_USER=myuser
```

### EGLFS Configuration

The installed EGLFS config at `/usr/local/share/wxdash/eglfs.json` targets `/dev/dri/card1`, which is correct for Pi 4 and Pi 5 with the Mesa V3D driver.

If the display is blank after starting the service:

1. List DRM device nodes: `ls /dev/dri/`
2. On some Pi OS versions the render node may be `card0` instead of `card1`.
3. Confirm the V3D node: `sudo drminfo /dev/dri/card1` should report the V3D driver.
4. If needed, edit `/usr/local/share/wxdash/eglfs.json` and change the `device` field to the correct node, then restart the service.

### systemd Service

```bash
# Reload unit files after install (if not using install-kiosk target)
sudo systemctl daemon-reload

# Enable auto-start at boot and start now
sudo systemctl enable wxdash.service
sudo systemctl start wxdash.service

# Check current status
systemctl status wxdash.service

# Follow live logs
journalctl -u wxdash.service -f

# Stop and disable
sudo systemctl stop wxdash.service
sudo systemctl disable wxdash.service
```

The service is configured with `Restart=always` so wxdash will restart automatically after a crash.

### Network and Hostname Resolution

The application connects to `weatherlinklive.local.cisien.com` — this is a DNS name, not a plain mDNS `.local` address.

Verify resolution on the Pi:

```bash
nslookup weatherlinklive.local.cisien.com
```

If the name does not resolve, add a static entry to `/etc/hosts`:

```
<IP_ADDRESS> weatherlinklive.local.cisien.com
```

The PurpleAir sensor is accessed at `http://10.1.255.41/json`. Verify the Pi can reach that address:

```bash
curl -s http://10.1.255.41/json | head -c 200
```

### Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| Black screen at boot | Wrong DRM device node in eglfs.json | Check `journalctl -u wxdash -b` for EGLFS errors; update `device` in eglfs.json |
| Permission denied on /dev/dri | User not in render/video groups | Run `groups $USER`; add missing groups with `usermod -aG` |
| QML module not found | QML_IMPORT_PATH not set or wrong path | Verify `QML_IMPORT_PATH` in service matches install prefix; check `ls /usr/local/qml/wxdash/` |
| No weather data | Hostname resolution or network failure | Run `nslookup weatherlinklive.local.cisien.com` and `curl http://10.1.255.41/json` from the Pi |
| High CPU / thermal throttle | Debug build in use | Rebuild with `-DCMAKE_BUILD_TYPE=Release` |

## License

MIT License — see LICENSE file for details.
