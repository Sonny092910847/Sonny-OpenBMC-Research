# Power Control Implementation for Console KVM

## 📁 檔案說明

本資料夾包含將 Server Power Operations 功能整合到 Console KVM 頁面的完整實作。

### 檔案列表

```
Power-Control-Implement/
├── KvmConsole-Modified.vue      ← 修改後的檔案（完整功能）
├── KvmConsole-Original.vue      ← 原始檔案（備份）
├── README.md                    ← 本檔案（總覽）
├── 安裝說明.md                  ← 如何安裝和使用
├── 測試說明.md                  ← 如何測試功能
└── 修改內容說明.md              ← 詳細的修改說明
```

---

## 🎯 功能說明

### 原始功能（保留）
- ✅ KVM Console 畫面顯示
- ✅ Ctrl+Alt+Delete 按鈕
- ✅ Open new tab 按鈕（僅在非全螢幕模式）
- ✅ KVM 連線狀態顯示

### 新增功能
- ✅ **電源狀態顯示**（Power Status）
  - 顯示主機當前電源狀態（On/Off/Unknown）
  - 即時狀態指示器（綠色/紅色/灰色）

- ✅ **Power On 按鈕**（主機關機時顯示）
  - 發送開機命令
  - 使用 Redfish API: `POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset`
  - ResetType: `On`

- ✅ **Reboot 下拉選單**（主機開機時顯示）
  - Orderly Reboot（優雅重啟）- 通知作業系統
  - Immediate Reboot（強制重啟）- 立即重啟
  - 包含確認對話框

- ✅ **Shutdown 下拉選單**（主機開機時顯示）
  - Orderly Shutdown（優雅關機）- 通知作業系統
  - Immediate Shutdown（強制關機）- 立即斷電
  - 包含確認對話框

- ✅ **操作狀態指示**
  - 操作進行中顯示 Spinner
  - 操作完成顯示 Toast 提示
  - 錯誤處理和提示

---

## ⚠️ 重要說明

### 版本資訊
- **Jenkins 映像檔版本：** OpenBMC v3.0.0-dev (styhead, BUILD_ID: 20251112010147)
- **本機版本：** OpenBMC v2.18
- **webui-vue 版本：** 最新版本（從 GitHub master branch）

### 版本差異影響
由於版本不匹配，建議使用**選項 1：開發服務器測試**方式，這樣前端可以連接任何版本的後端。

---

## 🚀 快速開始

### 方法 1：使用開發服務器（推薦）⭐

**優點：**
- ✅ 最快看到結果
- ✅ 修改立即生效（hot reload）
- ✅ 不需要重新編譯映像檔
- ✅ 適合快速測試

**步驟：**
```bash
# 1. 克隆 webui-vue（如果還沒有）
git clone https://github.com/openbmc/webui-vue.git
cd webui-vue

# 2. 安裝依賴
npm install

# 3. 套用修改（複製修改後的檔案）
cp /path/to/KvmConsole-Modified.vue \
   src/views/Operations/Kvm/KvmConsole.vue

# 4. 啟動開發服務器
npm run serve

# 5. 瀏覽器開啟
# http://localhost:8080/#/console/kvm
```

詳細步驟請參考 **《安裝說明.md》**

---

## 📊 修改內容摘要

### Template 修改
1. **新增電源狀態顯示區域**
   - Power Status 標籤和圖示
   - 即時更新主機狀態

2. **新增 Power Control 按鈕群組**
   - 根據主機狀態動態顯示不同按鈕
   - 使用 Bootstrap Vue 的 Dropdown 組件

3. **移除連線狀態限制**
   - 按鈕不再依賴 `v-if="isConnected"`
   - 即使 KVM 未連線，Power Control 仍可使用

### Script 修改
1. **新增 computed 屬性**
   - `hostStatus`: 取得主機電源狀態
   - `hostStatusIcon`: 電源狀態對應的圖示
   - `isOperationInProgress`: 是否正在執行操作

2. **新增 methods**
   - `powerOn()`: 開機
   - `confirmReboot(type)`: 重啟確認
   - `rebootServer(type)`: 執行重啟
   - `confirmShutdown(type)`: 關機確認
   - `shutdownServer(type)`: 執行關機

3. **新增 Icon 組件**
   - `IconPower`: 電源圖示
   - `IconRestart`: 重啟圖示

### Style 修改
1. **新增 CSS class**
   - `.power-control-buttons`: 按鈕群組樣式

---

## 🔌 API 端點

所有 Power Control 功能使用標準的 Redfish API：

| 功能 | Vuex Action | Redfish API | ResetType |
|------|------------|-------------|-----------|
| Power On | `controls/serverPowerOn` | `POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset` | `On` |
| Orderly Reboot | `controls/serverSoftReboot` | 同上 | `GracefulRestart` |
| Immediate Reboot | `controls/serverHardReboot` | 同上 | `ForceRestart` |
| Orderly Shutdown | `controls/serverSoftPowerOff` | 同上 | `GracefulShutdown` |
| Immediate Shutdown | `controls/serverHardPowerOff` | 同上 | `ForceOff` |

---

## 📸 預期效果

### 主機關機時
```
┌────────────────────────────────────────────────┐
│ Status: Connected    Power Status: Off         │
│ [Power On]  [Ctrl+Alt+Delete]                  │
└────────────────────────────────────────────────┘
```

### 主機開機時
```
┌────────────────────────────────────────────────┐
│ Status: Connected    Power Status: On          │
│ [Reboot ▼]  [Shutdown ▼]  [Ctrl+Alt+Delete]   │
│   └─ Orderly Reboot                            │
│   └─ Immediate Reboot                          │
└────────────────────────────────────────────────┘
```

---

## 🧪 測試清單

- [ ] Power On 功能
- [ ] Orderly Reboot 功能
- [ ] Immediate Reboot 功能
- [ ] Orderly Shutdown 功能
- [ ] Immediate Shutdown 功能
- [ ] 電源狀態即時更新
- [ ] 操作進行中的 Spinner 顯示
- [ ] Toast 提示訊息顯示
- [ ] 確認對話框功能
- [ ] 原有 KVM 功能不受影響
- [ ] Ctrl+Alt+Delete 仍正常運作

---

## 📞 支援

如有問題，請檢查：
1. **《安裝說明.md》** - 安裝步驟
2. **《測試說明.md》** - 測試方法
3. **《修改內容說明.md》** - 詳細修改說明

---

**版本：** 1.0.0
**日期：** 2024-11-03
**作者：** Claude Code Assistant
