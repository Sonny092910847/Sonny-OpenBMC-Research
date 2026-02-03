SUMMARY = "Sensor Monitor D-Bus Service"
DESCRIPTION = "A D-Bus service that monitors file values and sends threshold alarms"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig systemd

DEPENDS = " \
    boost \
    sdbusplus \
"

SRC_URI = "file://src/main.cpp \
           file://src/sensor_monitor.cpp \
           file://src/sensor_monitor.hpp \
           file://meson.build \
           file://training-sensor-monitor.service \
          "

S = "${WORKDIR}/sources" 
UNPACKDIR = "${S}"


SYSTEMD_SERVICE:${PN} = "training-sensor-monitor.service"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/training-sensor-monitor.service ${D}${systemd_system_unitdir}/
}
