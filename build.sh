#!/bin/bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build -j1
sudo cmake --install build --component kiosk
sudo cmake --build build --target install-kiosk
sudo systemctl restart wxdash
