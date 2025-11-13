# OpenBMC WebUI-Vue - KVM Console 電源操作功能

## 📋 專案概述

此專案為 OpenBMC webui-vue 的 KVM Console 頁面添加 **Server Power Operations**（伺服器電源操作）功能。

### 🎯 主要目標

在 KVM Console 視窗內部直接提供電源控制按鈕，讓使用者無需切換頁面即可執行伺服器電源管理操作。

---

## 🔧 修改檔案清單

### 1. **KvmConsole.vue**
📁 `src/views/Operations/Kvm/KvmConsole.vue`

#### ✨ 新增功能

- ✅ **Server Power Operations 控制面板**
  在 KVM Console 頁面頂部添加電源操作控制面板

- ✅ **6 個電源控制按鈕**：
  1. **Power On** - 開機（成功按鈕，綠色）
  2. **Graceful Restart** - 優雅重啟（警告按鈕，黃色）
  3. **Force Restart** - 強制重啟（警告按鈕，黃色）
  4. **Graceful Shutdown** - 優雅關機（次要按鈕，灰色）
  5. **Force Off** - 強制關閉（危險按鈕，紅色）
  6. **Power Cycle** - 電源循環（信息按鈕，藍色）

- ✅ **即時電源狀態顯示**
  透過 Redfish API 每 10 秒自動更新伺服器電源狀態

- ✅ **操作確認對話框**
  所有電源操作都需要使用者確認，防止誤操作

- ✅ **操作進行中狀態**
  操作執行期間自動禁用所有按鈕，並顯示載入動畫

- ✅ **無需 KVM 連接即可使用**
  **重要**：即使在 QEMU 環境中 iKVM 無法連接，電源操作按鈕仍然完全可用

#### 🎨 視覺設計

- 使用 Carbon Design System 圖示
- Bootstrap 4 樣式系統
- 響應式布局（支援不同螢幕尺寸）
- 按鈕懸停動畫效果
- 狀態徽章顯示（成功/失敗）

#### 🔌 API 整合

所有電源操作透過 **Redfish API** 執行：

```http
POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset/
Content-Type: application/json

{
  "ResetType": "On" | "GracefulRestart" | "ForceRestart" | "GracefulShutdown" | "ForceOff" | "PowerCycle"
}
```

#### 📝 程式碼註解

所有方法都包含詳細的 JSDoc 註解，說明功能和參數。

---

### 2. **vue.config.js**
📁 `vue.config.js`

#### 🛠️ 修復內容

**問題**：webpack-dev-server v4+ 棄用 `https` 選項
**錯誤訊息**：`'https' option is deprecated. Please use 'server.type' option.`

#### ✅ 解決方案

```javascript
// ❌ 舊版寫法（已棄用）
devServer: {
  https: true
}

// ✅ 新版寫法（webpack-dev-server 4.x+）
devServer: {
  server: {
    type: 'https'
  }
}
```

#### 🌐 代理配置

完整配置了前端到 OpenBMC 後端的代理：

- `/login` - 登入驗證
- `/api` - REST API
- `/redfish` - Redfish API（電源操作）
- `/download` - 檔案下載
- `/upload` - 檔案上傳
- `/subscribe` - WebSocket 訂閱

預設後端位置：`https://localhost:8443`
可透過環境變數 `BASE_URL` 自訂

---

## 🚀 使用方式

### 1️⃣ 將檔案複製到你的 webui-vue 專案

```bash
# 複製 KvmConsole.vue
cp src/views/Operations/Kvm/KvmConsole.vue ~/webui-vue/src/views/Operations/Kvm/

# 複製 vue.config.js
cp vue.config.js ~/webui-vue/
```

### 2️⃣ 安裝依賴（如果尚未安裝）

```bash
cd ~/webui-vue
npm install
```

### 3️⃣ 啟動開發伺服器

```bash
npm run serve
```

開發伺服器將啟動在：`https://localhost:8080`

### 4️⃣ 連接到 OpenBMC 後端

確保你的 OpenBMC 後端正在運行：

- **QEMU 模擬器**：`https://localhost:8443`
- **實體 BMC**：根據實際 IP 修改 `vue.config.js` 中的 `BASE_URL`

---

## 🌍 測試環境

### 系統環境
- **作業系統**：Ubuntu 22.04 LTS (VirtualBox)
- **Node.js**：v18.20.8
- **npm**：10.8.2

### OpenBMC 後端
- **平台**：romulus-bmc (AST2500)
- **版本**：3.0.0-dev (BUILD_ID: 20251112010147)
- **來源**：Jenkins 預編譯映像（2025/11/12）
- **端口**：
  - HTTPS：`localhost:8443`
  - HTTP：`localhost:8082`

### webui-vue 前端
- **來源**：GitHub master branch（最新版）
- **Vue 版本**：2.7.16
- **位置**：`~/webui-vue/`

---

## 🔍 功能特色

### ⚡ 無需 KVM 連接即可使用

這是本專案的**核心特色**！

在 QEMU 環境中，由於功能限制，iKVM（Intel KVM）通常無法建立連接。但是：

- ✅ 電源操作按鈕**不依賴** KVM 連接狀態
- ✅ 即使 KVM 顯示 "Disconnected"，電源操作仍然完全可用
- ✅ 按鈕狀態僅由「操作進行中」狀態控制，不受 KVM 連接影響

### 🔒 安全性設計

1. **雙重確認**：所有操作都需要點擊確認對話框
2. **操作鎖定**：執行操作期間自動禁用所有按鈕，防止重複提交
3. **錯誤處理**：完整的錯誤捕獲和使用者提示

### 📊 即時狀態監控

- 每 10 秒自動查詢電源狀態
- 操作完成後 2 秒自動刷新狀態
- 狀態顯示：**開機**、**關機**、**未知**

---

## 🐛 疑難排解

### 問題 1：npm run serve 啟動失敗

**錯誤訊息**：
```
'https' option is deprecated. Please use 'server.type' option.
```

**解決方案**：
確保你使用了本專案提供的 `vue.config.js`，已修復此問題。

---

### 問題 2：電源操作按鈕點擊無反應

**可能原因**：
1. 未正確連接到 OpenBMC 後端
2. 認證 Token 過期

**解決方案**：
1. 檢查 `vue.config.js` 中的 `proxy.target` 設定
2. 重新登入 webui-vue 介面

---

### 問題 3：電源狀態顯示為 "Unknown"

**可能原因**：
Redfish API 無法存取

**解決方案**：
1. 確認 OpenBMC 後端正在運行
2. 檢查瀏覽器控制台的網路請求錯誤
3. 確認後端 Redfish 服務已啟動

---

## 📚 技術架構

### 前端技術棧

- **Vue.js 2.7.16** - 漸進式 JavaScript 框架
- **Vuex** - 狀態管理
- **Bootstrap Vue** - UI 組件庫
- **Carbon Icons** - IBM 設計系統圖示
- **Webpack 5** - 模組打包工具

### 後端 API

- **Redfish API** - DMTF 標準的管理介面
  - 系統資訊：`GET /redfish/v1/Systems/system`
  - 電源操作：`POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset/`

### D-Bus 後端服務

電源操作最終透過 D-Bus 呼叫以下服務：

1. **Host 狀態管理**
   - 服務：`xyz.openbmc_project.State.Host`
   - 屬性：`RequestedHostTransition`
   - 值：`On`, `GracefulWarmReboot`, `ForceWarmReboot`, `Off`

2. **Chassis 狀態管理**
   - 服務：`xyz.openbmc_project.State.Chassis`
   - 屬性：`RequestedPowerTransition`
   - 值：`Off`

---

## 🎓 相關文件

### OpenBMC 官方文件
- [OpenBMC GitHub](https://github.com/openbmc/openbmc)
- [webui-vue GitHub](https://github.com/openbmc/webui-vue)
- [Redfish 規範](https://www.dmtf.org/standards/redfish)

### 測試參考
- 測試腳本：`OpenBMC v2.18/openbmc-test-automation/gui/gui_test/operations_menu/test_server_power_operations_sub_menu.robot`
- 後端實現：`OpenBMC v2.18/bmcweb/redfish-core/lib/systems.hpp`

---

## 📝 版本歷史

### v1.0.0 (2025-11-13)

#### ✅ 新增功能
- 在 KvmConsole.vue 中添加 Server Power Operations 控制面板
- 實現 6 個電源操作按鈕（Power On, Graceful Restart, Force Restart, Graceful Shutdown, Force Off, Power Cycle）
- 集成 Redfish API 電源控制
- 即時電源狀態監控
- 操作確認對話框

#### 🛠️ 修復問題
- 修復 vue.config.js 中 webpack-dev-server v4+ 的 HTTPS 配置問題
- 從 `devServer.https: true` 遷移到 `devServer.server.type: 'https'`

#### 🎨 設計改進
- 無論 KVM 連接狀態如何都顯示電源操作按鈕
- 完整的繁體中文註解和文件

---

## 👨‍💻 開發者

此專案為 OpenBMC webui-vue 的研究和增強專案。

---

## 📄 授權

本專案遵循 Apache License 2.0，與 OpenBMC 專案相同。

---

## 🙏 致謝

感謝 OpenBMC 社群提供的優秀開源專案和文件！

---

## 📞 聯絡方式

如有問題或建議，請透過 GitHub Issues 回報。

---

**🎉 祝你使用愉快！**
