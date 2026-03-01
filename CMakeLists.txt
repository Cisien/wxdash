cmake_minimum_required(VERSION 3.22)
project(wxdash LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compile warnings
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Qt6 REQUIRED COMPONENTS Core Network Test Quick Qml Svg)
qt_standard_project_setup()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

# --- Install rules (kiosk deployment) ---
include(GNUInstallDirs)

# Configure systemd service file (substitutes @CMAKE_INSTALL_PREFIX@)
configure_file(
    ${CMAKE_SOURCE_DIR}/assets/wxdash.service.in
    ${CMAKE_BINARY_DIR}/wxdash.service
    @ONLY
)

# Configure desktop entry (substitutes @CMAKE_INSTALL_PREFIX@)
configure_file(
    ${CMAKE_SOURCE_DIR}/assets/wxdash-kiosk.desktop.in
    ${CMAKE_BINARY_DIR}/wxdash-kiosk.desktop
    @ONLY
)

# Install systemd unit file
install(FILES ${CMAKE_BINARY_DIR}/wxdash.service
    DESTINATION lib/systemd/system
    COMPONENT kiosk
)

# Install desktop entry files
install(FILES
    assets/wxdash.desktop
    ${CMAKE_BINARY_DIR}/wxdash-kiosk.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    COMPONENT kiosk
)

# Install SVG icon to hicolor theme
install(FILES assets/wxdash.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    COMPONENT kiosk
)

# Install EGLFS KMS config for Pi 4/5
install(FILES assets/eglfs.json
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wxdash
    COMPONENT kiosk
)

# Convenience install-kiosk target: installs + enables systemd service
# Requires: sudo cmake --build build --target install-kiosk
add_custom_target(install-kiosk
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --component kiosk
    COMMAND systemctl daemon-reload
    COMMAND systemctl enable wxdash.service
    COMMENT "Installing wxdash kiosk and enabling systemd service (requires sudo)"
    VERBATIM
)
