SUMMARY = "TEST_Temp Sensor Service for L5 Training"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://test-temp-sensor.cpp \
           file://meson.build \
           file://test-temp-sensor.service"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig systemd

DEPENDS = "sdbusplus boost"

SYSTEMD_SERVICE:${PN} = "test-temp-sensor.service"
SYSTEMD_AUTO_ENABLE = "enable"

FILES:${PN} += "${systemd_system_unitdir}/test-temp-sensor.service"
