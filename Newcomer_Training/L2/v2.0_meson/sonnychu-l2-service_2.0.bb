SUMMARY = "SonnyChu L2 Training D-Bus Service (Meson Version)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://sonnychu-l2-service.cpp \
           file://meson.build \
           file://sonnychu-l2-service.service"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig systemd

DEPENDS = "sdbusplus boost"

SYSTEMD_SERVICE:${PN} = "sonnychu-l2-service.service"
SYSTEMD_AUTO_ENABLE = "enable"

FILES:${PN} += "${systemd_system_unitdir}/sonnychu-l2-service.service"
