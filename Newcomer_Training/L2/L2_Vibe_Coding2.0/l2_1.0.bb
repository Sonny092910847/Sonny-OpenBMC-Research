SUMMARY = "L2 Training D-Bus Monitor Service"
DESCRIPTION = "D-Bus service with file monitoring and threshold alerts"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = " \
    file://l2.cpp \
    file://meson.build \
    file://l2.service \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig
inherit obmc-phosphor-systemd

DEPENDS = " \
    boost \
    sdbusplus \
    phosphor-logging \
"

SYSTEMD_SERVICE:${PN} = "l2.service"
