SUMMARY = "L4 GPIO Monitor Service"
DESCRIPTION = "OpenBMC Training L4 - GPIO input monitoring service"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig systemd

DEPENDS = " \
    boost \
    libgpiod \
    phosphor-logging \
    sdbusplus \
"

RDEPENDS:${PN} = "libgpiod"

SRC_URI = " \
    file://l4.cpp \
    file://meson.build \
    file://l4.service \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

SYSTEMD_SERVICE:${PN} = "l4.service"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/l4.service ${D}${systemd_system_unitdir}/
}
