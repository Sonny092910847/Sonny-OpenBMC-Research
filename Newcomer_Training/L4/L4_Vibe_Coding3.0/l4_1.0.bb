SUMMARY = "L4 GPIO Monitor Service"
DESCRIPTION = "A service that monitors GPIO state changes and outputs logs"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

# Source files
SRC_URI = " \
    file://l4.cpp \
    file://meson.build \
    file://l4.service \
"

# Use UNPACKDIR instead of S = WORKDIR (golden-rules #7)
S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

# Dependencies
DEPENDS = " \
    boost \
    libgpiod \
    phosphor-logging \
    sdbusplus \
"

# Runtime dependencies
RDEPENDS:${PN} = "libgpiod"

# Inherit classes
inherit meson
inherit pkgconfig
inherit systemd

# Systemd configuration
SYSTEMD_SERVICE:${PN} = "l4.service"
SYSTEMD_AUTO_ENABLE = "enable"

# Install systemd service
do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/l4.service ${D}${systemd_system_unitdir}/
}

# Package files
FILES:${PN} += "${systemd_system_unitdir}/l4.service"
