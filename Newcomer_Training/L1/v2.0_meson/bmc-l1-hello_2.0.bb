SUMMARY = "BMC L1 Training - Hello Service (Meson Version)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://bmc_L1_ID835051.cpp \
           file://meson.build \
           file://bmc.L1.id835051.service"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson systemd

SYSTEMD_SERVICE:${PN} = "bmc.L1.id835051.service"
SYSTEMD_AUTO_ENABLE = "enable"

FILES:${PN} += "${systemd_system_unitdir}/bmc.L1.id835051.service"
