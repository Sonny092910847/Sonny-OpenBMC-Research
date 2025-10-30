# PPT 程式碼片段建議（架構 1）

## 📋 概覽

以下是從 `vm_websocket.hpp` 擷取的 5 個關鍵程式碼片段，按照邏輯順序排列。
每個片段都標示了需要用紅色框框或箭頭強調的重點。

---

## 🎯 片段 1：Handler 類別核心成員變數

**行號**: 178-184

**程式碼**:
```cpp
    boost::asio::readable_pipe pipeOut;   // 從 nbd-proxy 讀取
    boost::asio::writable_pipe pipeIn;    // 寫入到 nbd-proxy
    boost::process::v2::process proxy;    // nbd-proxy 子進程
    bool doingWrite{false};

    boost::beast::flat_static_buffer<nbdBufferSize> outputBuffer;  // Pipe → WebSocket
    boost::beast::flat_static_buffer<nbdBufferSize> inputBuffer;   // WebSocket → Pipe
```

### 🔴 紅色標示位置：

1. **紅框 1**: 框住 `pipeOut` 和 `pipeIn` 兩行
   - 註解：「Pipe 雙向通道」

2. **紅框 2**: 框住 `proxy` 那行
   - 註解：「nbd-proxy 子進程」

3. **紅框 3**: 框住 `outputBuffer` 和 `inputBuffer` 兩行
   - 註解：「雙緩衝區設計」

### 📊 視覺化建議：
```
pipeOut  ←────┐
pipeIn   ─────┤ [紅框] Pipe 雙向通道
              │
proxy    ─────→ [紅框] nbd-proxy 子進程

outputBuffer ──┐
inputBuffer  ──┤ [紅框] 雙緩衝區設計
```

---

## 🎯 片段 2：建構函式 - 啟動 nbd-proxy 子進程

**行號**: 57-62

**程式碼**:
```cpp
Handler(const std::string& media, boost::asio::io_context& ios) :
    pipeOut(ios), pipeIn(ios),
    proxy(ios, "/usr/bin/nbd-proxy", {media},
          boost::process::v2::process_stdio{
              .in = pipeIn, .out = pipeOut, .err = nullptr})
{}
```

### 🔴 紅色標示位置：

1. **紅框 1**: 框住整個 `proxy(...)` 初始化
   - 註解：「啟動 nbd-proxy」

2. **紅色箭頭 1**: 從 `pipeIn` 指向 `.in = pipeIn`
   - 標註：「設定為 stdin」

3. **紅色箭頭 2**: 從 `pipeOut` 指向 `.out = pipeOut`
   - 標註：「設定為 stdout」

### 📊 視覺化建議：
```
Handler(media, ios):
    pipeOut, pipeIn ← 初始化
    ↓
    proxy("/usr/bin/nbd-proxy")  ← [紅框] 啟動子進程
        .in  = pipeIn   ← [紅箭頭] 標準輸入
        .out = pipeOut  ← [紅箭頭] 標準輸出
```

---

## 🎯 片段 3：doWrite() - WebSocket → Pipe

**行號**: 117-123 (核心部分)

**程式碼**:
```cpp
pipeIn.async_write_some(
    inputBuffer.data(),
    [this, self(shared_from_this())](const boost::beast::error_code& ec,
                                     std::size_t bytesWritten) {
        BMCWEB_LOG_DEBUG("Wrote {}bytes", bytesWritten);
        doingWrite = false;
        inputBuffer.consume(bytesWritten);
```

### 🔴 紅色標示位置：

1. **紅框**: 框住 `pipeIn.async_write_some(inputBuffer.data(), ...)`
   - 註解：「寫入 pipe」

2. **紅色箭頭**: inputBuffer → pipeIn
   - 標註：「WebSocket 資料 → Pipe」

3. **紅框小**: 框住 `inputBuffer.consume(bytesWritten)`
   - 註解：「釋放已寫資料」

### 📊 視覺化建議：
```
[inputBuffer]  ─────→  pipeIn.async_write_some()
  (WebSocket)    [紅箭頭]        ↓
                              nbd-proxy

bytesWritten → inputBuffer.consume() [紅框]
```

---

## 🎯 片段 4：doRead() - Pipe → WebSocket

**行號**: 148-172 (核心部分)

**程式碼**:
```cpp
pipeOut.async_read_some(
    outputBuffer.prepare(bytes),
    [this, self(shared_from_this())](
        const boost::system::error_code& ec, std::size_t bytesRead) {
        BMCWEB_LOG_DEBUG("Read done.  Read {} bytes", bytesRead);

        outputBuffer.commit(bytesRead);
        std::string_view payload(
            static_cast<const char*>(outputBuffer.data().data()),
            bytesRead);
        session->sendBinary(payload);
        outputBuffer.consume(bytesRead);

        doRead();
    });
```

### 🔴 紅色標示位置：

1. **紅框 1**: 框住 `pipeOut.async_read_some(outputBuffer.prepare(bytes), ...)`
   - 註解：「從 pipe 讀取」

2. **紅色箭頭 1**: pipeOut → outputBuffer
   - 標註：「Pipe → 緩衝區」

3. **紅框 2**: 框住 `session->sendBinary(payload)`
   - 註解：「送回 WebSocket」

4. **紅色箭頭 2**: outputBuffer → session
   - 標註：「緩衝區 → WebSocket」

### 📊 視覺化建議：
```
nbd-proxy
    ↓
pipeOut.async_read_some()  [紅框]
    ↓ [紅箭頭]
[outputBuffer]
    ↓ [紅箭頭]
session->sendBinary()  [紅框]
    ↓
Browser (WebSocket)
```

---

## 🎯 片段 5：WebSocket onmessage 處理

**行號**: 604-620

**程式碼**:
```cpp
.onmessage([](crow::websocket::Connection& conn,
              const std::string& data, bool) {
    if (data.length() > handler->inputBuffer.capacity() -
                            handler->inputBuffer.size())
    {
        BMCWEB_LOG_ERROR("Buffer overrun when writing {} bytes",
                         data.length());
        conn.close("Buffer overrun");
        return;
    }

    size_t copied = boost::asio::buffer_copy(
        handler->inputBuffer.prepare(data.size()),
        boost::asio::buffer(data));
    handler->inputBuffer.commit(copied);
    handler->doWrite();
});
```

### 🔴 紅色標示位置：

1. **紅框 1**: 框住 `buffer_copy(handler->inputBuffer.prepare(...), buffer(data))`
   - 註解：「複製到 inputBuffer」

2. **紅色箭頭**: data → inputBuffer
   - 標註：「WebSocket 資料」

3. **紅框 2**: 框住 `handler->doWrite()`
   - 註解：「觸發寫入 pipe」

### 📊 視覺化建議：
```
WebSocket.onmessage(data)
    ↓ [紅箭頭]
buffer_copy(data → inputBuffer)  [紅框]
    ↓
handler->doWrite()  [紅框]
    ↓
Pipe (送給 nbd-proxy)
```

---

## 🎨 PPT 設計建議

### 投影片分配：

#### **投影片 1: 核心資料結構**
- 使用「片段 1」
- 標題：「Handler 類別：三個關鍵元件」
- 用三個紅框分別強調 Pipe、子進程、緩衝區

#### **投影片 2: 啟動子進程**
- 使用「片段 2」
- 標題：「建構函式：啟動 nbd-proxy 並設定 Pipe」
- 用紅色箭頭顯示 pipeIn/Out 如何連接到 stdin/stdout

#### **投影片 3: 上傳方向 (WebSocket → Pipe)**
- 合併使用「片段 5」和「片段 3」
- 標題：「資料上傳流程」
- 左邊放片段 5（WebSocket 接收），右邊放片段 3（寫入 Pipe）
- 用大的紅色箭頭連接兩個片段

#### **投影片 4: 下載方向 (Pipe → WebSocket)**
- 使用「片段 4」
- 標題：「資料下載流程」
- 用垂直的紅色箭頭顯示資料流：pipe → buffer → WebSocket

#### **投影片 5: 完整雙向流程（可選）**
- 不用程式碼，改用圖示
- 整合前面的流程，展示雙向同時進行

---

## 🎯 紅色標示重點總結

### 必須用紅框標示：
1. ✅ Pipe 變數（pipeIn, pipeOut）
2. ✅ 子進程變數（proxy）
3. ✅ 雙緩衝區（inputBuffer, outputBuffer）
4. ✅ async_write_some / async_read_some 呼叫
5. ✅ session->sendBinary() 呼叫
6. ✅ handler->doWrite() 呼叫

### 必須用紅色箭頭：
1. ✅ WebSocket data → inputBuffer
2. ✅ inputBuffer → pipeIn
3. ✅ pipeOut → outputBuffer
4. ✅ outputBuffer → WebSocket
5. ✅ pipeIn/Out 連接到 stdin/stdout

---

## 💡 額外建議

### 顏色方案：
- 🔴 紅色：關鍵操作、資料流向
- 🔵 藍色：WebSocket 相關
- 🟢 綠色：Pipe 相關
- 🟡 黃色：Buffer 相關

### 字體大小：
- 程式碼：建議 14-16pt（確保可讀性）
- 註解：建議 12-14pt
- 行號：可選，10pt

### 排版技巧：
1. 不要放太多行（每個片段最多 10-15 行）
2. 重要行可以加底色（淺黃或淺灰）
3. 保留適當縮排，保持可讀性
4. 註解可以用氣泡框（callout）呈現

---

## 📂 檔案位置

完整原始檔：`bmcweb/features/virtual_media/vm_websocket.hpp`
- 架構 1：第 54-190 行
- 重點行號已在上方各片段中標示

---

## 🔍 使用方式

1. 複製上述程式碼片段到 PPT
2. 使用 PPT 的「形狀」功能繪製紅框
3. 使用「箭頭」繪製資料流向
4. 使用「文字方塊」加上註解
5. 建議使用 Consolas 或 Monaco 等 monospace 字型

祝您 PPT 製作順利！🎉
