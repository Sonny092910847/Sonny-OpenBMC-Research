# Sonny OpenBMC Research

深入研究 OpenBMC 核心架構與實作的個人研究專案

## 📋 專案簡介

本專案收錄了 OpenBMC v2.18 及相關核心 repositories，用於深入學習和研究 OpenBMC 的架構、設計模式及最佳實踐。

**OpenBMC v2.18.0**
- 發布日期：2024年5月30日
- 基於：Yocto 5.2 "walnascar"
- Commit Hash: 05f7a3f

## 📁 專案結構

```
Sonny-OpenBMC-Research/
├── README.md
└── OpenBMC v2.18/
    ├── openbmc-core/              # OpenBMC v2.18 主專案
    ├── bmcweb/                    # BMC Web 服務與 Redfish API
    ├── sdbusplus/                 # D-Bus C++ 綁定
    ├── phosphor-dbus-interfaces/  # D-Bus 介面定義
    ├── phosphor-logging/          # 日誌系統
    ├── phosphor-host-ipmid/       # Host IPMI Daemon
    ├── phosphor-net-ipmid/        # Network IPMI Daemon
    ├── phosphor-state-manager/    # 狀態管理服務
    ├── phosphor-hwmon/            # 硬體監控
    ├── phosphor-fan-presence/     # 風扇控制
    ├── phosphor-power/            # 電源管理
    ├── phosphor-user-manager/     # 使用者管理
    ├── dbus-sensors/              # D-Bus 感測器
    ├── entity-manager/            # 實體管理器
    ├── jsnbd/                     # Virtual Media (NBD Proxy)
    ├── docs/                      # OpenBMC 官方文檔
    ├── openbmc-test-automation/   # 測試自動化框架
    ├── openbmc-tools/             # 開發工具集
    └── ... (共 42 個核心 repositories)
```

## 🎯 研究目標

### 1️⃣ 架構理解
- OpenBMC 整體架構
- D-Bus 通訊機制
- Phosphor 框架設計模式
- Yocto/BitBake 建構系統

### 2️⃣ 核心服務分析
- **bmcweb**: Redfish API 實作、HTTP/WebSocket 服務
- **phosphor-***: 各種 BMC 服務的實作細節
- **sdbusplus**: D-Bus C++ 綁定的使用方式

### 3️⃣ 硬體管理
- 感測器數據處理
- 風扇與熱管理
- 電源控制
- LED 管理

### 4️⃣ 管理介面
- IPMI 協定實作
- Redfish RESTful API
- WebUI 整合

## 📚 包含的核心 Repositories

### Web 與 API 服務
- `bmcweb` - BMC Web 服務器，提供 Redfish API

### D-Bus 核心
- `sdbusplus` - D-Bus C++ 綁定庫
- `phosphor-dbus-interfaces` - D-Bus 介面 YAML 定義
- `phosphor-objmgr` - D-Bus 物件管理器

### IPMI 服務
- `phosphor-host-ipmid` - Host IPMI Daemon
- `phosphor-net-ipmid` - Network IPMI Daemon
- `ipmitool` - IPMI 命令列工具
- `phosphor-ipmi-blobs` - IPMI BLOB 協定
- `phosphor-ipmi-flash` - IPMI Flash 更新

### 狀態與生命週期管理
- `phosphor-state-manager` - 系統狀態管理
- `phosphor-watchdog` - Watchdog 服務
- `phosphor-post-code-manager` - POST Code 管理

### 韌體與軟體管理
- `phosphor-bmc-code-mgmt` - BMC 韌體更新
- `phosphor-software-manager` - 軟體管理服務

### 硬體監控
- `phosphor-hwmon` - 硬體監控（溫度、電壓等）
- `dbus-sensors` - D-Bus 感測器框架
- `entity-manager` - 硬體實體管理
- `phosphor-nvme` - NVMe 設備監控

### 電源與風扇控制
- `phosphor-power` - 電源管理
- `phosphor-fan-presence` - 風扇偵測與控制
- `phosphor-pid-control` - PID 溫度控制

### 日誌與除錯
- `phosphor-logging` - 事件與日誌系統
- `phosphor-debug-collector` - Debug 資料收集
- `phosphor-hostlogger` - Host 主機日誌
- `phosphor-sel-logger` - System Event Log

### 用戶與安全
- `phosphor-user-manager` - 用戶帳號管理
- `phosphor-certificate-manager` - 憑證管理

### 網路管理
- `phosphor-networkd` - 網路配置服務

### LED 與顯示
- `phosphor-led-manager` - LED 控制管理
- `phosphor-led-sysfs` - LED sysfs 介面

### 其他核心服務
- `phosphor-time-manager` - 時間管理
- `phosphor-inventory-manager` - 硬體清單管理
- `phosphor-host-postd` - Host POST Daemon
- `phosphor-mboxd` - Mailbox Daemon
- `phosphor-snmp` - SNMP 代理
- `phosphor-virtual-sensor` - 虛擬感測器
- `phosphor-health-monitor` - 健康監控

### Virtual Media
- `jsnbd` - Network Block Device Proxy

### 文檔與工具
- `docs` - OpenBMC 官方文檔
- `openbmc-test-automation` - 自動化測試框架
- `openbmc-tools` - 開發工具集
- `meta-phosphor` - Phosphor Yocto meta layer

## 🔍 研究重點

### D-Bus 通訊模式
研究 OpenBMC 如何使用 D-Bus 進行服務間通訊：
- Interface 定義（phosphor-dbus-interfaces）
- C++ 綁定實作（sdbusplus）
- 實際應用範例

### Redfish API 實作
深入了解 bmcweb 如何實作 Redfish 標準：
- RESTful API 設計
- WebSocket 支援
- 認證與授權機制

### 感測器架構
學習 OpenBMC 的感測器框架：
- HWMon 整合
- D-Bus 感測器
- Entity Manager 配置

### 狀態機設計
研究各種服務的狀態管理：
- 系統狀態轉換
- 電源狀態控制
- 錯誤處理流程

## 🛠️ 使用方式

### 瀏覽程式碼
```bash
cd "OpenBMC v2.18"

# 查看主專案
cd openbmc-core

# 研究 Redfish API 實作
cd bmcweb

# 了解 D-Bus 介面定義
cd phosphor-dbus-interfaces
```

### 查閱文檔
```bash
cd "OpenBMC v2.18/docs"
# OpenBMC 官方文檔包含架構說明、設計文件等
```

## 📖 學習路徑

### 第一階段：基礎架構
1. 閱讀 `docs/` 中的架構文檔
2. 理解 D-Bus 在 OpenBMC 中的角色
3. 研究 `phosphor-dbus-interfaces` 的介面定義

### 第二階段：核心服務
1. 分析 `bmcweb` 的 Redfish 實作
2. 研究 `phosphor-state-manager` 的狀態管理
3. 了解 IPMI 服務的實作方式

### 第三階段：硬體整合
1. 學習 `phosphor-hwmon` 的感測器整合
2. 研究 `phosphor-fan-presence` 的風扇控制
3. 分析 `dbus-sensors` 與 `entity-manager`

### 第四階段：進階主題
1. 韌體更新機制（phosphor-bmc-code-mgmt）
2. Virtual Media 實作（jsnbd）
3. 測試與自動化（openbmc-test-automation）

## 🔗 相關資源

- [OpenBMC 官網](https://github.com/openbmc/openbmc)
- [OpenBMC 文檔](https://github.com/openbmc/docs)
- [Redfish 標準](https://www.dmtf.org/standards/redfish)
- [D-Bus 規範](https://www.freedesktop.org/wiki/Software/dbus/)

## 📊 統計資訊

- **OpenBMC 版本**: v2.18.0
- **收錄 Repositories**: 42 個核心專案
- **專案範圍**: 原生 OpenBMC（不含 ODM/IC 廠商特定版本）

## 📝 研究筆記

（在此記錄研究過程中的發現與筆記）

## 📄 授權

本專案用於個人研究學習。所有 OpenBMC 相關程式碼遵循其原始授權條款。

---

**研究開始日期**: 2025-10-23
**最後更新**: 2025-10-23
