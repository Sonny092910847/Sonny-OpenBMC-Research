# 🪟 Windows 用戶專用操作指南

## 📍 您的檔案位置

所有檔案都已經複製到：

```
<您的專案根目錄>\Power-Control-修改檔案\
```

這個資料夾透過 **Docker Volume** 掛載，Windows 可以直接存取！

---

## 🎯 在 Windows 上用 VSCode 開啟檔案

### 方法 1：開啟整個資料夾（推薦）

1. **開啟 VSCode**
   - 在 Windows 開始選單搜尋 "VSCode"
   - 或雙擊桌面上的 VSCode 圖示

2. **開啟專案資料夾**
   ```
   File → Open Folder...
   或按快捷鍵: Ctrl + K, Ctrl + O
   ```

3. **選擇您的專案位置**
   ```
   導航到您的 Docker 專案掛載位置
   例如：
   - D:\OpenBMC\Sonny-OpenBMC-Research\
   - 或您實際的掛載路徑
   ```

4. **在左側檔案總管找到資料夾**
   ```
   Power-Control-修改檔案/
   ├── KvmConsole-Power-Control-詳細註解版.vue  ← 點這個！
   ├── KvmConsole-with-power-control.vue
   ├── monitor-power-control.sh
   ├── simple-monitor.sh
   ├── Power-Control-監控指南.md
   ├── 快速開始指南.md
   └── README.md
   ```

---

### 方法 2：直接開啟單個檔案

1. **在 VSCode 中**
   ```
   File → Open File...
   或按快捷鍵: Ctrl + O
   ```

2. **導航到檔案**
   ```
   <專案路徑>\Power-Control-修改檔案\KvmConsole-Power-Control-詳細註解版.vue
   ```

3. **點擊開啟**

---

### 方法 3：從 Windows 檔案總管拖曳

1. **開啟 Windows 檔案總管**
   ```
   按 Win + E
   ```

2. **導航到您的專案資料夾**
   ```
   D:\OpenBMC\Sonny-OpenBMC-Research\Power-Control-修改檔案\
   （或您的實際路徑）
   ```

3. **找到 `KvmConsole-Power-Control-詳細註解版.vue`**

4. **拖曳到 VSCode 視窗**

---

### 方法 4：右鍵選單（如果已安裝 VSCode 右鍵選單）

1. **在檔案總管中**
   ```
   導航到: Power-Control-修改檔案\
   ```

2. **在檔案上按右鍵**
   ```
   選擇: Open with Code
   或: 使用 Code 開啟
   ```

---

## 🔍 如何找到您的專案在 Windows 上的位置

### 選項 1：檢查 Docker Volume 設定

1. **開啟 Docker Desktop**

2. **找到您的容器**
   ```
   Containers → 找到正在運行的 OpenBMC 容器
   ```

3. **查看 Volumes**
   ```
   點擊容器 → Inspect → Mounts
   看 "Source" 欄位（Windows 路徑）
   ```

### 選項 2：使用 docker inspect 命令

1. **開啟 Windows PowerShell 或 CMD**

2. **執行以下命令**
   ```powershell
   # 列出所有容器
   docker ps

   # 查看容器掛載資訊（替換 <容器ID>）
   docker inspect <容器ID或名稱> | findstr "Source"
   ```

3. **找到包含 "Sonny-OpenBMC-Research" 的路徑**

### 選項 3：在容器內查看（如果其他方法失敗）

如果您不確定 Windows 上的路徑，可以從容器複製出來：

1. **開啟 Windows PowerShell 或 CMD**

2. **執行 docker cp 命令**
   ```powershell
   # 先找到容器 ID
   docker ps

   # 從容器複製到 Windows 桌面
   docker cp <容器ID>:/home/user/Sonny-OpenBMC-Research/Power-Control-修改檔案 C:\Users\<您的用戶名>\Desktop\Power-Control-修改檔案
   ```

3. **檔案會出現在您的桌面上**

---

## 📖 推薦閱讀順序

### 第 1 步：了解檔案結構
```
開啟: README.md
閱讀時間: 5 分鐘
```

### 第 2 步：快速入門
```
開啟: 快速開始指南.md
閱讀時間: 10 分鐘
重點: 了解正確的修改流程
```

### 第 3 步：查看程式碼
```
開啟: KvmConsole-Power-Control-詳細註解版.vue
閱讀時間: 20-30 分鐘
建議: 從頭到尾仔細看註解
```

### 第 4 步：學習監控方法
```
開啟: Power-Control-監控指南.md
閱讀時間: 15 分鐘
重點: 了解如何追蹤按鈕點擊後的通訊
```

---

## 🎨 VSCode 推薦設定

### 安裝實用的擴充套件

1. **Vue Language Features (Volar)**
   - Vue 3 官方語言支援
   - 自動完成、語法高亮

2. **ESLint**
   - JavaScript/Vue 程式碼檢查

3. **Prettier - Code formatter**
   - 自動格式化程式碼

4. **Material Icon Theme**
   - 更好看的檔案圖示

5. **GitLens**（如果使用 Git）
   - 查看程式碼修改歷史

### VSCode 設定建議

1. **啟用自動換行**
   ```
   View → Word Wrap
   或按 Alt + Z
   ```

2. **調整字體大小**
   ```
   File → Preferences → Settings
   搜尋: Font Size
   建議: 14-16
   ```

3. **顯示空白字元**（推薦）
   ```
   View → Render Whitespace
   這樣可以看到空格和 Tab
   ```

---

## 🔍 如何比較修改前後的差異

### 方法 1：使用 VSCode Diff 功能

1. **開啟原始檔案**（如果有）
   ```
   例如: KvmConsole.vue.backup
   ```

2. **開啟修改後的檔案**
   ```
   KvmConsole-Power-Control-詳細註解版.vue
   ```

3. **開啟命令面板**
   ```
   按 Ctrl + Shift + P
   ```

4. **輸入並選擇**
   ```
   File: Compare Active File With...
   ```

5. **選擇要比較的檔案**

### 方法 2：使用 Git（如果專案有 Git）

```bash
# 在 VSCode 終端執行
git diff --no-index 原始檔案 修改後檔案
```

---

## 💡 Windows 常見問題

### Q1: 找不到 Power-Control-修改檔案 資料夾？

**解決方法：**
1. 確認 Docker 容器正在運行
2. 重新整理檔案總管（按 F5）
3. 檢查 Docker Volume 設定
4. 使用 `docker cp` 命令直接複製（見上方說明）

---

### Q2: VSCode 開啟檔案時出現亂碼？

**解決方法：**
1. 在 VSCode 右下角點擊編碼（例如 "UTF-8"）
2. 選擇 "Reopen with Encoding"
3. 選擇 "UTF-8"

---

### Q3: 檔案是唯讀的，無法編輯？

**原因：** 檔案權限問題（Docker 容器內是 root 建立的）

**解決方法：**

**方案 A: 在 Windows 修改權限**
```
檔案上按右鍵 → 內容 → 安全性 → 編輯
給您的使用者完全控制權限
```

**方案 B: 使用 docker cp 複製一份**
```powershell
docker cp <容器ID>:/home/user/Sonny-OpenBMC-Research/Power-Control-修改檔案 C:\Temp\
```
然後編輯 `C:\Temp\` 中的檔案

---

### Q4: 如何在 Windows 執行 .sh 腳本檔？

**答：** `.sh` 檔案是 Linux Shell 腳本，需要在容器內執行

**步驟：**
1. 使用 SSH 連線到 BMC（QEMU）
2. 上傳腳本：
   ```bash
   scp monitor-power-control.sh root@<BMC_IP>:/tmp/
   ```
3. 執行：
   ```bash
   ssh root@<BMC_IP>
   bash /tmp/monitor-power-control.sh
   ```

或使用 Windows 的 Git Bash / WSL 來執行（需要連線到 BMC）

---

## 📱 Windows 終端工具推薦

### SSH 連線工具（連接到 QEMU BMC）

1. **PuTTY**
   - 免費、輕量
   - 下載：https://www.putty.org/

2. **MobaXterm**（推薦！）
   - 功能強大
   - 內建 X11、SFTP
   - 下載：https://mobaxterm.mobatek.net/

3. **Windows Terminal + OpenSSH**
   - 現代化介面
   - Windows 10/11 內建

### 檔案傳輸工具（SCP/SFTP）

1. **WinSCP**
   - 免費、易用
   - 圖形化界面
   - 下載：https://winscp.net/

2. **FileZilla**
   - 支援 SFTP
   - 下載：https://filezilla-project.org/

---

## 🎯 下一步建議

### 現在可以做的事：

1. ✅ **用 VSCode 開啟詳細註解版檔案**
   - 仔細閱讀註解
   - 理解 Power Control 的運作流程

2. ✅ **閱讀操作手冊**
   - 快速開始指南.md
   - Power-Control-監控指南.md

3. ✅ **規劃測試環境**
   - 準備 SSH 工具
   - 了解如何監控通訊

### 等待編譯完成後再做：

4. ⏳ **測試原始版本**
5. ⏳ **套用修改**
6. ⏳ **重新編譯**
7. ⏳ **測試 Power Control 功能**

---

## 📞 需要協助？

如果在 Windows 上操作遇到任何問題，請提供：
1. 您使用的 Windows 版本（Win 10/11）
2. Docker Desktop 版本
3. 遇到的具體問題或錯誤訊息

我會協助您解決！

---

**祝您開發順利！🎉**
