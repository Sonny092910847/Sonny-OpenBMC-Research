SUMMARY = "TEST_Temp OEM IPMI Command for L5 Training Task 2"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://test-temp-oem.cpp \
           file://meson.build"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig

DEPENDS = "phosphor-ipmi-host phosphor-logging openssl"

FILES:${PN} += "/usr/lib/ipmid-providers/libtest-temp-oem.so"
