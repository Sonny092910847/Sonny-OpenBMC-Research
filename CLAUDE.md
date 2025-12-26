# CLAUDE.md - OpenBMC Newcomer Training

## 學員資訊
- 姓名：Sonny Chu
- 訓練項目：OpenBMC 新人訓 L1-L13
- 目前進度：L5 進行中（IPMI Sensor, SEL, FRU, OEM Commands）

## 開發環境

### BHS OpenBMC 環境
- 版本：bhs-0.80-981-g4ebff01312
- Codename：mickledore
- Machine：intel-ast2600
- Flash 大小：64MB

### 路徑配置
- Source: `~/bhs-openbmc-project/bhs-openbmc/`
- Build: `~/bhs-openbmc-project/bhs-openbmc/build/`
- Image: `~/bhs-openbmc-project/bhs-openbmc/build/tmp/deploy/images/intel-ast2600/`
- 訓練資料夾: `~/Newcomer_training/`

## 常用指令

### 進入 Build 環境（每次開新 terminal 必須執行）
```bash
cd ~/bhs-openbmc-project/bhs-openbmc && source oe-init-build-env
```

### 編譯指令
```bash
# 完整 image
bitbake intel-platforms

# 單一 package
bitbake <package-name>

# 清除後重新編譯
bitbake -c cleanall <package-name>
bitbake <package-name>
```

### QEMU 啟動
```bash
/home/sonny/bhs-openbmc-project/bhs-openbmc/build/tmp/work/x86_64-linux/qemu-system-native/8.0.3-r0/build/qemu-system-arm \
  -m 1G -M intel-ast2600 -nographic \
  -drive file=/home/sonny/bhs-openbmc-project/bhs-openbmc/build/tmp/deploy/images/intel-ast2600/image-mtd,format=raw,if=mtd \
  -net nic \
  -net user,hostfwd=:127.0.0.1:2228-:22,hostfwd=:127.0.0.1:2446-:443,hostname=qemu
```

### SSH/SCP 連線
```bash
# SSH 連線（密碼：0penBmc）
ssh -p 2228 root@127.0.0.1

# 傳檔案到 QEMU
scp -P 2228 <local-file> root@127.0.0.1:<remote-path>
```

## 開發規範

### 檔案結構
新增的 service/程式碼應遵循以下結構：
```
Newcomer_Training/
├── L5/
│   └── v1.0_meson/
│       ├── files/
│       │   ├── xxx.cpp
│       │   ├── meson.build
│       │   └── xxx.service
│       └── xxx_1.0.bb
```

### BitBake Recipe 標準格式
```bitbake
SUMMARY = "Description"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://xxx.cpp \
           file://meson.build \
           file://xxx.service"

S = "${WORKDIR}"

inherit meson systemd

SYSTEMD_SERVICE:${PN} = "xxx.service"
SYSTEMD_AUTO_ENABLE = "enable"
```

### Debug 流程
1. 編譯錯誤：檢查 recipe 路徑和 dependencies
2. Runtime 錯誤：用 `journalctl -u <service>` 查看 log
3. D-Bus 問題：用 `busctl` 檢查 service 狀態
4. Kernel/DTS 修改：使用 devtool 和產生 patch

### 重要技術筆記
- **GPIO 事件模式 vs 輪詢模式**：libgpiod 事件模式(EVENT_BOTH_EDGES)會持續佔用 GPIO，QEMU 環境需改用輪詢模式
- **D-Bus 架構**：services 發布標準 interfaces，Redfish 和 IPMI 自動消費
- **devtool 工作流程**：用於 kernel 相關修改，需產生完整路徑的 patch

## GitHub 設定
- Repository: Sonny-OpenBMC-Research
- Branch: OpenBMC-Fii-新人訓
- Git user.name: Sonny Chu
- Git user.email: jasongod5736@gmail.com

## 作業類型判斷
| 作業類型 | 上傳方式 |
|----------|----------|
| 修改現有檔案 | 上傳 Patch |
| 新增 Service/程式 | 上傳完整程式碼（Meson 結構） |
| 混合型 | Patch + 程式碼 |

## 報告格式要求
- 使用雙語（中文/英文）
- 表格標題：藍色背景 #D9E2F3
- 程式碼區塊：灰色背景 #F2F2F2
- 中文字體：Microsoft JhengHei
- 需包含完整截圖

## 重要提醒
- 編譯完整 image 需要較長時間，優先使用單一 package 編譯
- QEMU 測試時 SSH port 是 2228
- 修改 recipe 後需要 `bitbake -c cleanall` 再重新編譯
- 每次開新 terminal 都要 `source oe-init-build-env`
