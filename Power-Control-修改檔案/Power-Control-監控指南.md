# 🎯 Power Control 按鈕監控完整指南

## 📋 目錄
1. [方案 A vs 方案 B 差異](#方案差異)
2. [監控視窗設定](#監控視窗設定)
3. [詳細操作步驟](#詳細操作步驟)
4. [預期輸出範例](#預期輸出範例)

---

## 1. 方案差異

### 方案 A：簡單雙按鈕
```
工具列: [Power On] [Power Off] [Ctrl+Alt+Del] [開啟新分頁]
```
- ✅ 簡單直覺
- ✅ 快速操作
- ❌ 功能有限（只有開/關）

### 方案 B：完整下拉選單
```
工具列: [電源控制 ▼] [Ctrl+Alt+Del] [開啟新分頁]
         └─ 開機
         └─ 優雅關機
         └─ 強制關機
         └─ 優雅重啟
         └─ 強制重啟
```
- ✅ 功能完整（5種操作）
- ✅ 專業界面
- ❌ 需要兩次點擊

**建議：** 先用方案 A 測試功能，確認無誤後再升級到方案 B。

---

## 2. 監控視窗設定

### 需要的視窗

```
┌─────────────────────────┬─────────────────────────┐
│                         │                         │
│  視窗 1: 瀏覽器 WebUI   │  視窗 2: SSH 終端       │
│  - Console 分頁 (F12)   │  監控腳本執行中...      │
│  - Network 分頁         │                         │
│                         │  🌐 [bmcweb] ...        │
│  [Power On] ← 點這裡    │  🔌 [D-Bus] ...         │
│                         │  ⚙️  [State Mgr] ...    │
│                         │                         │
└─────────────────────────┴─────────────────────────┘
```

---

## 3. 詳細操作步驟

### 步驟 1: 修改 webui-vue 源碼

**在您的開發機器上：**

```bash
cd <您的 webui-vue 專案目錄>

# 備份原始檔案
cp src/views/Operations/Kvm/KvmConsole.vue src/views/Operations/Kvm/KvmConsole.vue.backup

# 下載修改後的檔案
# (假設您已經從 /tmp/KvmConsole-with-power-control.vue 複製內容)
```

**或直接修改：**

在 `src/views/Operations/Kvm/KvmConsole.vue` 中：

1. 在第 17-36 行之間插入 Power Control 按鈕
2. 在 components 中加入 `IconPower`
3. 在 methods 中加入 `powerOn()` 和 `powerOff()` 方法

詳細代碼請參考 `/tmp/KvmConsole-with-power-control.vue`

### 步驟 2: 編譯並部署 WebUI

```bash
# 安裝依賴（如果還沒安裝）
npm install

# 本地測試
npm run serve
# 瀏覽器開啟: http://localhost:8080

# 或構建生產版本
npm run build
# 輸出會在 dist/ 目錄
```

### 步驟 3: 部署到 QEMU/BMC

**方法 1: 直接複製 dist 內容到 BMC**
```bash
# 將 dist/ 目錄複製到 BMC
scp -r dist/* root@<BMC_IP>:/usr/share/www/

# 重啟 bmcweb
ssh root@<BMC_IP>
systemctl restart bmcweb
```

**方法 2: 重新構建 OpenBMC 映像檔（完整方法）**
```bash
# 在 Yocto 構建環境中
bitbake webui-vue -c cleansstate
bitbake webui-vue
bitbake obmc-phosphor-image
```

### 步驟 4: 設定監控環境

#### 4A. 使用完整監控腳本（推薦）

**上傳腳本到 BMC：**
```bash
scp /tmp/monitor-power-control.sh root@<BMC_IP>:/tmp/
```

**SSH 連線並執行：**
```bash
ssh root@<BMC_IP>
chmod +x /tmp/monitor-power-control.sh
/tmp/monitor-power-control.sh
```

您會看到：
```
🌐 [bmcweb] 日誌訊息...
🔌 [D-Bus] D-Bus 通訊訊息...
⚙️  [State Manager] 狀態管理器訊息...
```

#### 4B. 使用簡化監控命令

**直接在 SSH 執行：**
```bash
ssh root@<BMC_IP>
journalctl -u bmcweb -u xyz.openbmc_project.State.Host -f --no-pager
```

### 步驟 5: 開啟瀏覽器並準備測試

1. **開啟 WebUI：**
   ```
   https://<BMC_IP>/console/kvm
   ```

2. **開啟開發者工具（F12）：**
   - 切換到 **Console** 分頁
   - 同時開啟 **Network** 分頁（勾選 "Preserve log"）

### 步驟 6: 執行測試

1. **在 WebUI 點擊 [Power On] 按鈕**

2. **觀察輸出：**

#### 📺 瀏覽器 Console 分頁會顯示：
```javascript
🔵 ========== Power On 按鈕被點擊 ==========
📍 位置: KvmConsole.vue - powerOn() 方法
⏰ 時間: 2025-11-03T10:15:23.456Z

📤 第1步: 準備呼叫 Vuex Store Action
   Action 名稱: controls/serverPowerOn

📤 第2步: 發送 dispatch 到 Vuex Store

✅ 成功: Vuex Action 執行完成
📝 說明: 此 Action 會執行以下步驟:
   1. 設定 isOperationInProgress = true
   2. 呼叫 serverPowerChange({ ResetType: "On" })
   3. 發送 HTTP POST 到:
      URL: /redfish/v1/Systems/system/Actions/ComputerSystem.Reset
      Body: {"ResetType":"On"}
   4. 等待伺服器狀態變更為 "on"
   5. 設定 isOperationInProgress = false

🔍 提示: 請查看 Network 分頁以確認 HTTP 請求
🔍 提示: 請查看 SSH 終端機以確認 bmcweb 日誌
```

#### 📺 瀏覽器 Network 分頁會顯示：
```http
Request URL: https://192.168.1.100/redfish/v1/Systems/system/Actions/ComputerSystem.Reset
Request Method: POST
Status Code: 200 OK

Request Headers:
  Content-Type: application/json
  X-Auth-Token: 7b8e9f1a2c3d4e5f...

Request Payload:
  {
    "ResetType": "On"
  }

Response:
  (空 body，200 表示成功)
```

#### 📺 SSH 終端會顯示（使用完整監控腳本）：
```
🌐 [bmcweb] Nov 03 10:15:23 bmc bmcweb[1234]: POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset
🌐 [bmcweb] Nov 03 10:15:23 bmc bmcweb[1234]: Request body: {"ResetType":"On"}
🌐 [bmcweb] Nov 03 10:15:23 bmc bmcweb[1234]: processComputerSystemResetActionPost(): ResetType=On
🌐 [bmcweb] Nov 03 10:15:23 bmc bmcweb[1234]: Setting D-Bus property: RequestedHostTransition

🔌 [D-Bus] method call sender=:1.88 -> destination=xyz.openbmc_project.State.Host
🔌 [D-Bus]    interface=org.freedesktop.DBus.Properties
🔌 [D-Bus]    member=Set
🔌 [D-Bus]    string "RequestedHostTransition"
🔌 [D-Bus]    variant string "xyz.openbmc_project.State.Host.Transition.On"

⚙️  [State Manager] Nov 03 10:15:23 bmc phosphor-host-state-manager[5678]: Property set: RequestedHostTransition
⚙️  [State Manager] Nov 03 10:15:23 bmc phosphor-host-state-manager[5678]: Transitioning from Off to On
⚙️  [State Manager] Nov 03 10:15:24 bmc phosphor-host-state-manager[5678]: CurrentHostState = TransitioningToOn
⚙️  [State Manager] Nov 03 10:15:25 bmc phosphor-host-state-manager[5678]: CurrentHostState = Running
```

---

## 4. 預期輸出範例

### 完整的通訊流程時間軸

```
時間軸: 按下 Power On 按鈕後的事件序列

T+0ms     [前端] 按鈕點擊事件觸發
          └─ @click="powerOn" 被呼叫

T+10ms    [前端] powerOn() 方法執行
          └─ console.log 輸出到瀏覽器 Console

T+20ms    [前端] dispatch('controls/serverPowerOn')
          └─ Vuex Store Action 被呼叫

T+30ms    [前端] ControlStore.serverPowerOn() 執行
          └─ 設定 isOperationInProgress = true
          └─ 呼叫 serverPowerChange({ ResetType: "On" })

T+50ms    [前端] HTTP POST 請求發送
          └─ URL: /redfish/v1/Systems/system/Actions/ComputerSystem.Reset
          └─ 可在 Network 分頁看到

T+100ms   [後端] bmcweb 接收到請求
          └─ handleComputerSystemResetActionPost() 被呼叫
          └─ journalctl 輸出: "POST /redfish/v1/Systems/..."

T+120ms   [後端] bmcweb 解析 ResetType
          └─ processComputerSystemResetActionPost()
          └─ journalctl 輸出: "ResetType=On"

T+150ms   [後端] bmcweb 轉換到 D-Bus 命令
          └─ 轉換: "On" → "xyz.openbmc_project.State.Host.Transition.On"
          └─ journalctl 輸出: "Setting D-Bus property"

T+200ms   [D-Bus] setDbusProperty() 呼叫
          └─ Service: xyz.openbmc_project.State.Host
          └─ Object: /xyz/openbmc_project/state/host0
          └─ Property: RequestedHostTransition
          └─ dbus-monitor 輸出: "method call ... Set ..."

T+250ms   [後端] phosphor-state-manager 接收到 D-Bus 訊息
          └─ Host::requestedHostTransition() 被呼叫
          └─ journalctl 輸出: "Property set: RequestedHostTransition"

T+300ms   [後端] 狀態管理器執行轉換
          └─ CurrentHostState: Off → TransitioningToOn
          └─ journalctl 輸出: "Transitioning from Off to On"

T+2000ms  [後端] 電源控制完成
          └─ CurrentHostState: TransitioningToOn → Running
          └─ journalctl 輸出: "CurrentHostState = Running"

T+2500ms  [前端] checkForServerStatus 偵測到狀態變更
          └─ isOperationInProgress = false
          └─ 顯示成功提示訊息
```

---

## 5. 常見問題排除

### Q1: 瀏覽器 Console 沒有看到日誌？

**檢查：**
```javascript
// 確認 console.log 沒有被過濾
// 在 Console 分頁，確保 "All levels" 被選中
```

### Q2: Network 分頁沒有看到 POST 請求？

**檢查：**
- 是否勾選了 "Preserve log"
- 是否有認證錯誤（401）？檢查 X-Auth-Token

### Q3: SSH 終端沒有輸出？

**檢查服務狀態：**
```bash
systemctl status bmcweb
systemctl status xyz.openbmc_project.State.Host

# 手動查看最近的日誌
journalctl -u bmcweb -n 50 --no-pager
```

### Q4: 按鈕點擊沒有反應？

**檢查：**
```javascript
// 在瀏覽器 Console 手動測試
this.$store.dispatch('controls/serverPowerOn')

// 檢查 Vuex Store 是否已載入
console.log(this.$store.state.controls)
```

---

## 6. 進階：使用 curl 直接測試 API

**不透過 WebUI，直接測試 Redfish API：**

```bash
# 1. 登入取得 Token
curl -k -H "Content-Type: application/json" -X POST \
  https://<BMC_IP>/login \
  -d '{"username": "root", "password": "0penBmc"}' \
  -c cookies.txt

# 2. 測試 Power On
curl -k -b cookies.txt -H "Content-Type: application/json" -X POST \
  https://<BMC_IP>/redfish/v1/Systems/system/Actions/ComputerSystem.Reset \
  -d '{"ResetType": "On"}'

# 3. 查詢當前狀態
curl -k -b cookies.txt \
  https://<BMC_IP>/redfish/v1/Systems/system \
  | jq '.PowerState, .Status.State'
```

---

## 7. 總結

### 您的按鈕會觸發以下連線：

```
┌─────────────────┐
│  前端 Vue.js    │  按鈕點擊
└────────┬────────┘
         │
         ↓
┌─────────────────┐
│  Vuex Store     │  狀態管理
└────────┬────────┘
         │
         ↓ HTTP POST
┌─────────────────┐
│  bmcweb         │  Redfish API Server
└────────┬────────┘
         │
         ↓ D-Bus
┌─────────────────┐
│  State Manager  │  phosphor-state-manager
└────────┬────────┘
         │
         ↓
┌─────────────────┐
│  硬體/QEMU模擬  │  實際電源控制
└─────────────────┘
```

### 需要開啟的視窗：

**最簡配置（2個視窗）：**
1. 瀏覽器（WebUI + F12 開發者工具）
2. SSH 終端（執行監控腳本）

**完整配置（4個視窗）：**
1. 瀏覽器 WebUI
2. SSH - bmcweb 日誌
3. SSH - D-Bus 監控
4. SSH - State Manager 日誌

---

## 8. 檔案清單

本指南相關的檔案：

```
/tmp/KvmConsole-with-power-control.vue   ← 修改後的 Vue 組件
/tmp/monitor-power-control.sh            ← 完整監控腳本
/tmp/simple-monitor.sh                   ← 簡化監控腳本
/tmp/Power-Control-監控指南.md           ← 本檔案
```

---

**祝測試順利！如有問題請隨時詢問。** 🎉
