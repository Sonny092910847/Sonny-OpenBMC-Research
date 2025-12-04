SUMMARY = "BMC L1 Training - Hello Service"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://bmc_L1_ID835051.cpp \
           file://bmc.L1.id835051.service"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

do_compile() {
    ${CXX} ${CXXFLAGS} ${LDFLAGS} ${S}/bmc_L1_ID835051.cpp -o bmc-l1-hello
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/bmc-l1-hello ${D}${bindir}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/bmc.L1.id835051.service ${D}${systemd_system_unitdir}
}

inherit systemd
SYSTEMD_SERVICE:${PN} = "bmc.L1.id835051.service"
