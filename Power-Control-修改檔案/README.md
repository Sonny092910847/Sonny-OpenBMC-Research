# Power Control 修改檔案說明

## 📁 本資料夾包含的檔案

### 1. Vue 組件檔案（兩個版本）

#### ⭐ KvmConsole-Power-Control-詳細註解版.vue（推薦！）
- **大小：** 26 KB
- **特色：** 超詳細的中文註解，每一行都有說明
- **適合：** 初學者、想理解每個細節的開發者
- **用途：** 用來替換原始的 KvmConsole.vue

#### KvmConsole-with-power-control.vue
- **大小：** 11 KB
- **特色：** 簡潔版，註解適中
- **適合：** 熟悉 Vue.js 的開發者
- **用途：** 快速套用修改

---

### 2. 監控腳本

#### monitor-power-control.sh（完整版）
- **大小：** 2.6 KB
- **功能：** 同時監控 bmcweb、D-Bus、State Manager
- **輸出：** 彩色標籤（🌐 [bmcweb]、🔌 [D-Bus]、⚙️ [State Mgr]）
- **使用場景：** 想看到所有後端通訊細節

#### simple-monitor.sh（簡化版）
- **大小：** 660 bytes
- **功能：** 一行命令監控關鍵日誌
- **使用場景：** 快速測試

---

### 3. 操作手冊

#### Power-Control-監控指南.md
- **大小：** 12 KB
- **內容：**
  - 完整的操作步驟
  - 預期輸出範例
  - 問題排除
  - 進階 curl 測試命令

#### 快速開始指南.md
- **大小：** 8.6 KB
- **內容：**
  - 快速參考
  - 正確的修改流程
  - 如何還原
  - 常見問題

---

## 🚀 快速開始

### 步驟 1: 用 VSCode 開啟檔案

**在 Windows 上：**
1. 開啟 VSCode
2. File → Open Folder
3. 選擇您的專案資料夾（Docker Volume 掛載的位置）
4. 導航到 `Power-Control-修改檔案/`
5. 開啟 `KvmConsole-Power-Control-詳細註解版.vue`

**或直接開啟：**
```
File → Open File
選擇: <專案路徑>\Power-Control-修改檔案\KvmConsole-Power-Control-詳細註解版.vue
```

### 步驟 2: 閱讀操作手冊

建議先閱讀：
1. `快速開始指南.md` - 了解整體流程
2. `Power-Control-監控指南.md` - 學習如何監控通訊

### 步驟 3: 等待編譯完成

⚠️ **重要：** 請等待第一次 `bitbake obmc-phosphor-image` 編譯完成後再開始修改！

---

## 📝 修改流程（編譯完成後）

```
1. 測試原始版本
   └─ 確保 WebUI 和 KVM 功能正常

2. 找到 webui-vue 源碼位置
   └─ tmp/work/.../webui-vue/git/src/views/Operations/Kvm/

3. 備份原始檔案
   └─ cp KvmConsole.vue KvmConsole.vue.backup

4. 套用修改
   └─ cp <此資料夾>/KvmConsole-Power-Control-詳細註解版.vue <目標>/KvmConsole.vue

5. 重新編譯
   └─ bitbake webui-vue -c cleansstate
   └─ bitbake webui-vue
   └─ bitbake obmc-phosphor-image

6. 測試功能
   └─ 使用監控腳本追蹤通訊
```

---

## 🔄 如何還原

### 方法 1: 使用備份（最快）
```bash
cp KvmConsole.vue.backup KvmConsole.vue
```

### 方法 2: 重新編譯（徹底）
```bash
bitbake webui-vue -c cleanall
bitbake webui-vue
```

---

## 📊 檔案比較

| 檔案 | 註解程度 | 檔案大小 | 適合對象 |
|------|---------|---------|---------|
| 詳細註解版 | ⭐⭐⭐⭐⭐ | 26 KB | 初學者 |
| 簡潔版 | ⭐⭐⭐ | 11 KB | 有經驗者 |

---

## 💡 提示

- 建議使用 **VSCode 的 Diff 功能** 比較修改前後的差異
- 所有 console.log 都可以在瀏覽器開發者工具看到
- 監控腳本需要在 BMC 上執行（SSH 連線後）

---

## 📞 需要協助？

如果在使用過程中遇到任何問題，請隨時詢問！

---

**祝您開發順利！🎉**
