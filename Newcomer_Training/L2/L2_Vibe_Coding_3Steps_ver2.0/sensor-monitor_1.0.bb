SUMMARY = "OpenBMC Sensor Monitor Service"
DESCRIPTION = "D-Bus service for monitoring sensor values from file and triggering threshold alarms"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

# Source files
SRC_URI = " \
    file://sensor_monitor.cpp \
    file://meson.build \
    file://xyz.openbmc_project.SensorMonitor.service \
"

# Use UNPACKDIR pattern (BitBake 2.12+ requirement)
S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

# Dependencies
DEPENDS = " \
    boost \
    phosphor-logging \
    sdbusplus \
    systemd \
"

# Runtime dependencies
RDEPENDS:${PN} = " \
    phosphor-logging \
    sdbusplus \
"

# Inherit classes
inherit meson pkgconfig systemd

# Systemd configuration
SYSTEMD_SERVICE:${PN} = "xyz.openbmc_project.SensorMonitor.service"
SYSTEMD_AUTO_ENABLE = "enable"
