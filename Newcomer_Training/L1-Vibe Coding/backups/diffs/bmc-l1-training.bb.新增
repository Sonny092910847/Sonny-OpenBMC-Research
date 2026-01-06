# === L1: BMC Training Recipe START ===
SUMMARY = "BMC L1 Training Program"
DESCRIPTION = "A simple C++ program for BMC L1 Training - ID 835051"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = " \
    file://bmc_L1_ID835051.cpp \
    file://bmc.L1.id835051.service \
"

S = "${WORKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} = "bmc.L1.id835051.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_compile() {
    ${CXX} ${CXXFLAGS} ${LDFLAGS} -o bmc_L1_ID835051 ${WORKDIR}/bmc_L1_ID835051.cpp
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 bmc_L1_ID835051 ${D}${bindir}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bmc.L1.id835051.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${systemd_system_unitdir}/bmc.L1.id835051.service"
# === L1: BMC Training Recipe END ===
