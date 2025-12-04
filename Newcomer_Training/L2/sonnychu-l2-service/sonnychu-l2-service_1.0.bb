SUMMARY = "SonnyChu L2 Training D-Bus Service"
DESCRIPTION = "A D-Bus service for OpenBMC L2 training with file monitoring and threshold alarm"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://sonnychu-l2-service.cpp \
           file://meson.build \
           file://sonnychu-l2-service.service"

S = "${WORKDIR}/sources-unpack"

inherit meson pkgconfig systemd

DEPENDS = "sdbusplus boost"

SYSTEMD_SERVICE:${PN} = "sonnychu-l2-service.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/sonnychu-l2-service.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${systemd_system_unitdir}/sonnychu-l2-service.service"
