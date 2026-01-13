SUMMARY = "L1 Exercise 1 Training Application"
DESCRIPTION = "Simple C++ application that prints message every 5 seconds"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = " \
    file://L1_ex1.cpp \
    file://meson.build \
    file://L1_ex1.service \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig
inherit obmc-phosphor-systemd

SYSTEMD_SERVICE:${PN} = "L1_ex1.service"
