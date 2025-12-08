SUMMARY = "GPIO Monitor Service (Meson Version)"
DESCRIPTION = "Monitor GPIOA1 and output state change messages using polling mode"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://gpio-monitor.cpp \
           file://meson.build \
           file://gpio-monitor.service"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig systemd

DEPENDS = "libgpiod"

SYSTEMD_SERVICE:${PN} = "gpio-monitor.service"
SYSTEMD_AUTO_ENABLE = "enable"

FILES:${PN} += "${systemd_system_unitdir}/gpio-monitor.service"
