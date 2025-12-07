SUMMARY = "SonnyChu L2 Training D-Bus Service"                                                              #基本資訊,授權
DESCRIPTION = "A D-Bus service for OpenBMC L2 training with file monitoring and threshold alarm"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://sonnychu-l2-service.cpp \                                                                 //原始碼來源(去哪裡拿檔案)
           file://meson.build \
           file://sonnychu-l2-service.service"

S = "${WORKDIR}/sources-unpack"                                                                             #複製SRC_URI的檔案到暫存

inherit meson pkgconfig systemd                                                                             #使用meson來編譯, pkg-config工具來找函式庫位置, 使用systemd方法

DEPENDS = "sdbusplus boost"                                                                                 #編譯時依賴的套件

SYSTEMD_SERVICE:${PN} = "sonnychu-l2-service.service"                                                       #啟用systemd
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {                                                                                       #若將sonnychu-l2-service.service包在meson中可以省略 (install_data()函數)
    install -d ${D}${systemd_system_unitdir} 
    install -m 0644 ${S}/sonnychu-l2-service.service ${D}${systemd_system_unitdir}/    
}

FILES:${PN} += "${systemd_system_unitdir}/sonnychu-l2-service.service"                                      #宣告sonnychu-l2-service.service屬於此recipe。     #還是需要，sonnychu-l2-service.service是純文字設定檔
