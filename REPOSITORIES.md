# OpenBMC Repositories 索引

本文件詳細列出所有包含在此研究專案中的 OpenBMC repositories。

## 📊 總覽

- **總數**: 42 個核心 repositories
- **版本**: OpenBMC v2.18.0
- **類型**: 原生 OpenBMC（不含 vendor-specific repositories）

## 📑 Repositories 分類清單

### 🌐 Web 與 API (1)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **bmcweb** | BMC Web 服務器，實作 Redfish API、WebSocket、REST API | C++, Boost.Beast, HTTP/2 |

### 🔌 D-Bus 核心 (3)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **sdbusplus** | systemd D-Bus C++ 綁定庫 | C++, D-Bus |
| **phosphor-dbus-interfaces** | D-Bus 介面的 YAML 定義檔 | YAML, D-Bus |
| **phosphor-objmgr** | D-Bus 物件管理器 | C++, D-Bus |

### 📡 IPMI 服務 (5)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-host-ipmid** | Host 端點 IPMI 命令處理 | C++, IPMI |
| **phosphor-net-ipmid** | 網路 IPMI (RMCP+) 實作 | C++, IPMI |
| **ipmitool** | IPMI 命令列工具 | C, IPMI |
| **phosphor-ipmi-blobs** | IPMI BLOB 傳輸協定 | C++, IPMI |
| **phosphor-ipmi-flash** | 透過 IPMI 進行韌體更新 | C++, IPMI |

### 🔄 狀態與生命週期管理 (3)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-state-manager** | BMC、Host、Chassis 狀態管理 | C++, systemd |
| **phosphor-watchdog** | Watchdog 服務，監控系統健康 | C++, Linux Watchdog |
| **phosphor-post-code-manager** | POST Code 管理與記錄 | C++ |

### 💾 韌體與軟體管理 (2)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-bmc-code-mgmt** | BMC 韌體更新與管理 | C++, systemd |
| **phosphor-software-manager** | 軟體版本管理 | C++ |

### 🌡️ 硬體監控與感測器 (4)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-hwmon** | Linux HWMon 感測器對應到 D-Bus | C++, sysfs |
| **dbus-sensors** | 現代化的 D-Bus 感測器框架 | C++ |
| **entity-manager** | 硬體實體配置管理器 | C++, JSON |
| **phosphor-nvme** | NVMe 設備監控 | C++, NVMe-MI |

### ⚡ 電源與熱管理 (3)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-power** | 電源供應器監控與管理 | C++ |
| **phosphor-fan-presence** | 風扇偵測、監控與控制 | C++ |
| **phosphor-pid-control** | PID 溫度控制器 | C++ |

### 📝 日誌與除錯 (4)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-logging** | 事件與日誌系統，錯誤日誌管理 | C++, systemd-journal |
| **phosphor-debug-collector** | Debug 資料與 Dump 收集 | C++ |
| **phosphor-hostlogger** | Host 主機控制台日誌記錄 | C++ |
| **phosphor-sel-logger** | System Event Log (SEL) 記錄器 | C++ |

### 🔐 用戶與安全管理 (2)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-user-manager** | 使用者帳號與權限管理 | C++, PAM |
| **phosphor-certificate-manager** | SSL/TLS 憑證管理 | C++, OpenSSL |

### 🌐 網路管理 (1)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-networkd** | 網路介面配置服務 | C++, systemd-networkd |

### 💡 LED 管理 (2)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-led-manager** | LED 群組控制與管理 | C++ |
| **phosphor-led-sysfs** | LED sysfs 介面支援 | C++ |

### 🕒 其他核心服務 (5)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-time-manager** | 系統時間與 NTP 管理 | C++ |
| **phosphor-inventory-manager** | 硬體清單管理 | C++ |
| **phosphor-host-postd** | Host POST 訊息處理 | C++ |
| **phosphor-mboxd** | Mailbox daemon（用於 Flash 存取） | C |
| **phosphor-snmp** | SNMP 代理服務 | C++ |

### 🎯 進階功能 (2)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **phosphor-virtual-sensor** | 虛擬感測器（計算型感測器） | C++ |
| **phosphor-health-monitor** | 系統健康監控 | C++ |

### 📀 Virtual Media (1)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **jsnbd** | Network Block Device Proxy for Virtual Media | JavaScript, NBD |

### 📚 文檔與工具 (4)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **docs** | OpenBMC 官方文檔、架構設計文件 | Markdown |
| **openbmc-test-automation** | Robot Framework 測試自動化 | Python, Robot Framework |
| **openbmc-tools** | 開發與除錯工具集 | Python, Shell |
| **meta-phosphor** | Phosphor Yocto meta layer | BitBake |

### 🏗️ 主專案 (1)

| Repository | 說明 | 主要技術 |
|------------|------|----------|
| **openbmc-core** | OpenBMC v2.18.0 主要建構系統 | Yocto, BitBake |

## 🔍 學習建議路徑

### 入門級（1-2 週）
1. **docs** - 理解 OpenBMC 整體架構
2. **phosphor-dbus-interfaces** - 了解 D-Bus 介面定義
3. **sdbusplus** - 學習 D-Bus C++ 綁定

### 初級（2-4 週）
4. **phosphor-state-manager** - 狀態管理機制
5. **phosphor-logging** - 日誌系統
6. **phosphor-hwmon** - 簡單的感測器實作

### 中級（1-2 個月）
7. **bmcweb** - Redfish API 實作
8. **phosphor-host-ipmid** - IPMI 協定
9. **dbus-sensors** + **entity-manager** - 現代感測器架構

### 高級（2-3 個月）
10. **phosphor-bmc-code-mgmt** - 韌體更新機制
11. **phosphor-fan-presence** + **phosphor-pid-control** - 複雜的控制系統
12. **openbmc-test-automation** - 測試框架

## 📊 技術堆疊統計

- **主要語言**: C++ (35), Python (3), JavaScript (1)
- **通訊協定**: D-Bus, Redfish, IPMI, SNMP, NBD
- **框架**: Phosphor, systemd, Yocto/BitBake
- **標準**: Redfish, IPMI 2.0, DMTF

## 🔗 快速參考

### 想了解...

- **Redfish API**: 參考 `bmcweb/`
- **D-Bus 介面**: 參考 `phosphor-dbus-interfaces/`
- **感測器**: 參考 `phosphor-hwmon/`, `dbus-sensors/`
- **IPMI**: 參考 `phosphor-host-ipmid/`, `phosphor-net-ipmid/`
- **狀態管理**: 參考 `phosphor-state-manager/`
- **韌體更新**: 參考 `phosphor-bmc-code-mgmt/`
- **風扇控制**: 參考 `phosphor-fan-presence/`, `phosphor-pid-control/`
- **日誌系統**: 參考 `phosphor-logging/`

---

*最後更新: 2025-10-23*
