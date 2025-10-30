# PPT 建議內容：vm_websocket.hpp（架構 1）

## 📄 投影片標題
**WebSocket 到 nbd-proxy 的橋樑**
或
**bmcweb Handler：WebSocket ↔ Pipe 轉接器**

---

## 📊 投影片內容建議

### 1️⃣ 【問題】協定轉換的挑戰

當使用者從瀏覽器上傳 ISO 時，面臨協定不匹配：
- **瀏覽器**: 使用 WebSocket 協定
- **nbd-proxy**: 期望從 stdin/stdout 讀寫資料（標準輸入輸出）

**需要一個中間層來轉換！**

---

### 2️⃣ 【解決方案】bmcweb Handler 類別

**檔案位置**: `bmcweb/features/virtual_media/vm_websocket.hpp:54-190`

**核心角色**: WebSocket ↔ Pipe 雙向轉接器

```cpp
class Handler {
    boost::asio::readable_pipe pipeOut;   // 從 nbd-proxy 讀取
    boost::asio::writable_pipe pipeIn;    // 寫入到 nbd-proxy
    boost::process::v2::process proxy;    // nbd-proxy 子進程

    // 雙向緩衝區
    flat_static_buffer<nbdBufferSize> outputBuffer;  // pipe → WebSocket
    flat_static_buffer<nbdBufferSize> inputBuffer;   // WebSocket → pipe
};
```

---

### 3️⃣ 【架構設計】三個核心職責

#### A. 啟動 nbd-proxy 子進程
```cpp
proxy(ios, "/usr/bin/nbd-proxy", {media},
      boost::process::v2::process_stdio{
          .in = pipeIn,    // 設定 stdin
          .out = pipeOut,  // 設定 stdout
          .err = nullptr
      })
```
**說明**: 將 pipe 作為 nbd-proxy 的 stdin/stdout

---

#### B. 雙向資料轉發

**為什麼需要兩個緩衝區？**
- 上傳（瀏覽器 → 伺服器）和下載（伺服器 → 瀏覽器）可能同時發生
- 兩個獨立的 buffer 避免資料衝突
- 每個方向獨立處理，效能更好

**資料流向**：

```
┌─────────────┐                    ┌─────────────┐
│   Browser   │                    │  nbd-proxy  │
└──────┬──────┘                    └──────┬──────┘
       │                                  │
   WebSocket                            Pipe
       │                                  │
       ↓                                  ↓
  ┌────────────────────────────────────────┐
  │         bmcweb Handler                 │
  │                                        │
  │  [inputBuffer]  ─→ [pipeIn]  ─→      │ (上傳方向)
  │                                        │
  │  [outputBuffer] ←─ [pipeOut] ←─      │ (下載方向)
  └────────────────────────────────────────┘
```

---

#### C. 非同步讀寫操作

**寫入 pipe (doWrite)**:
```cpp
pipeIn.async_write_some(
    inputBuffer.data(),
    [callback...](const boost::beast::error_code& ec,
                  std::size_t bytesWritten) {
        inputBuffer.consume(bytesWritten);  // 釋放已寫入的資料
        doWrite();  // 繼續處理剩餘資料
    });
```

**從 pipe 讀取 (doRead)**:
```cpp
pipeOut.async_read_some(
    outputBuffer.prepare(bytes),
    [callback...](const boost::system::error_code& ec,
                  std::size_t bytesRead) {
        outputBuffer.commit(bytesRead);
        session->sendBinary(payload);  // 送到 WebSocket
        outputBuffer.consume(bytesRead);
        doRead();  // 繼續讀取
    });
```

**關鍵**: 使用 Boost.ASIO 非同步 I/O，不會阻塞主執行緒

---

### 4️⃣ 【路由註冊】WebSocket 端點

```cpp
BMCWEB_ROUTE(app, "/vm/0/0")
    .privileges({{"ConfigureComponents", "ConfigureManager"}})
    .websocket()
    .onopen([](crow::websocket::Connection& conn) {
        handler = std::make_shared<Handler>(media, getIoContext());
        handler->connect();
    })
    .onclose([](crow::websocket::Connection& conn, const std::string&) {
        handler->doClose();  // 終止 nbd-proxy
        handler.reset();
    })
    .onmessage([](crow::websocket::Connection& conn,
                  const std::string& data, bool) {
        // 資料寫入 inputBuffer
        size_t copied = boost::asio::buffer_copy(
            handler->inputBuffer.prepare(data.size()),
            boost::asio::buffer(data));
        handler->inputBuffer.commit(copied);
        handler->doWrite();  // 觸發寫入 pipe
    });
```

**端點**: `/vm/0/0`
**觸發時機**: 在 WebUI 點擊「掛載」時，JavaScript 連接此 WebSocket

---

### 5️⃣ 【緩衝區大小設計】

```cpp
static constexpr auto nbdBufferSize = (128 * 1024 + 16) * 4;
```

**計算邏輯**:
- NBD 協定最大訊息: 128KB (資料) + 16 bytes (標頭)
- × 4 倍緩衝
- **總計**: 524,352 bytes (約 512KB)

**為什麼 ×4？**
- 允許同時處理多個 NBD 請求
- 減少因緩衝區滿而阻塞的機會

---

### 6️⃣ 【完整資料流】

```
┌──────────┐  1.WebSocket  ┌──────────┐  2.Pipe   ┌───────────┐
│ Browser  │ ─────────────→ │  bmcweb  │ ─────────→│ nbd-proxy │
│          │                │ Handler  │  (stdio)  │           │
│          │ ←───────────── │          │ ←───────── │           │
└──────────┘  4.WebSocket  └──────────┘  3.Pipe   └───────────┘
                                                          │
                                                    Unix Socket
                                                          ↓
                                                    nbd-client
```

**步驟說明**:
1. 瀏覽器透過 WebSocket 送出 ISO 區塊資料
2. Handler 寫入 `inputBuffer`，透過 `pipeIn` 送給 nbd-proxy
3. nbd-proxy 處理後，回應資料透過 `pipeOut` 送回
4. Handler 從 `outputBuffer` 讀取，透過 WebSocket 送回瀏覽器

---

### 7️⃣ 【關鍵特性總結】

| 特性 | 說明 |
|------|------|
| **子進程管理** | 動態啟動/終止 nbd-proxy |
| **雙向轉發** | WebSocket ↔ Pipe 協定轉換 |
| **非同步 I/O** | 不阻塞主執行緒，效能高 |
| **雙緩衝區** | 上傳下載獨立，避免衝突 |
| **大緩衝區** | 512KB，支援高速資料傳輸 |
| **錯誤處理** | 任一端斷線，自動清理資源 |

---

### 8️⃣ 【與下一層的銜接】

**這一層的輸出 = 下一層的輸入**

```
vm_websocket.hpp (這頁)
    ↓
    透過 Pipe (stdin/stdout)
    ↓
nbd-proxy.c (下一頁)
```

**nbd-proxy 看到的**:
- 從 stdin 讀取資料（實際來自 bmcweb 的 pipeIn）
- 寫入 stdout（實際送回 bmcweb 的 pipeOut）
- **對 nbd-proxy 來說，就像在處理標準輸入輸出**

---

## 🎯 投影片設計建議

### 視覺化元素：
1. **架構圖**: 強調 WebSocket - bmcweb - Pipe - nbd-proxy 的關係
2. **資料流動圖**: 用箭頭標示雙向資料流
3. **程式碼片段**: 關鍵的 async_write_some / async_read_some
4. **對比**: 可以加一個小 box 說明「架構 2 已棄用」

### 重點提示：
- 使用不同顏色區分上傳/下載方向
- 標示 buffer 名稱（`inputBuffer` / `outputBuffer`）
- 強調「Pipe」是關鍵通訊機制

---

## ⚠️ 常見混淆點（可加入補充說明）

**Q: 為什麼不讓 bmcweb 直接建立 Unix Socket？**
A: 這是架構 2 的設計，但因缺少後端支援已被棄用。架構 1 透過 nbd-proxy 中間層，責任分離更清楚。

**Q: Pipe 和 Unix Socket 有什麼差別？**
A:
- Pipe: 單向，用於父子進程通訊（bmcweb ↔ nbd-proxy）
- Unix Socket: 雙向，用於本地進程通訊（nbd-proxy ↔ nbd-client）

---

## 📚 參考資料

- 程式碼: `bmcweb/features/virtual_media/vm_websocket.hpp` (第 54-190 行)
- 編譯選項: `meson.options` 中的 `vm-websocket` (預設啟用)
- Boost.Process v2: https://www.boost.org/doc/libs/1_83_0/doc/html/process.html
- Boost.ASIO: https://www.boost.org/doc/libs/1_83_0/doc/html/boost_asio.html
