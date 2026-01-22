SUMMARY = "L4 Training GPIO Monitor Service"
DESCRIPTION = "D-Bus service for monitoring GPIOA1 state changes"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = " \
    file://l4.cpp \
    file://meson.build \
    file://l4.service \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig
inherit obmc-phosphor-systemd

DEPENDS = " \
    boost \
    sdbusplus \
    phosphor-logging \
    libgpiod \
"

SYSTEMD_SERVICE:${PN} = "l4.service"
