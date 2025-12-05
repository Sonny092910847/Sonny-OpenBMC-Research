SUMMARY = "GPIO Monitor Service"
DESCRIPTION = "Monitor GPIOA1 and output state change messages"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://gpio-monitor.cpp \
           file://meson.build \
           file://gpio-monitor.service"

S = "${WORKDIR}/sources-unpack"

inherit meson pkgconfig systemd

DEPENDS = "libgpiod"

SYSTEMD_SERVICE:${PN} = "gpio-monitor.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_unpack:append() {
    import shutil
    import os
    
    workdir = d.getVar('WORKDIR')
    s = d.getVar('S')
    
    os.makedirs(s, exist_ok=True)
    
    for f in ['gpio-monitor.cpp', 'meson.build', 'gpio-monitor.service']:
        src = os.path.join(workdir, f)
        dst = os.path.join(s, f)
        if os.path.exists(src):
            shutil.copy2(src, dst)
}

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/gpio-monitor.service ${D}${systemd_system_unitdir}/
}
