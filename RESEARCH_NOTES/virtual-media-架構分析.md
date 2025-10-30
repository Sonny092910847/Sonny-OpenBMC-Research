# Virtual Media 架構分析：vm_websocket.hpp 的兩種實作

## 📅 研究日期
2025-10-30

## 🎯 研究發現

`bmcweb/features/virtual_media/vm_websocket.hpp` 包含了**兩種互斥的架構實作**，透過編譯時選項決定使用哪一種。

## ⚠️ **重要：架構 2 已被棄用！**

**從 `bmcweb/meson.options` 分析發現：**

- ✅ **架構 1 (vm-websocket)**: `value: 'enabled'` - **唯一實際使用的架構**
- ❌ **架構 2 (vm-nbdproxy)**: **整個選項被註解掉** - **視為死代碼 (dead code)**

### 官方註解說明：
```
# BMCWEB_NBDPROXY
# if you use this option and are seeing this comment, please comment here:
# https://github.com/openbmc/bmcweb/issues/188 and put forward your intentions
# for this code.  At this point, no daemon has been upstreamed that implements
# this interface, so for the moment this appears to be dead code;  In leiu of
# removing it, it has been disabled to try to give those that use it the
# opportunity to upstream their backend implementation
```

### 關鍵原因：
1. **缺少後端實作**: "no daemon has been upstreamed that implements this interface"
2. **視為死代碼**: "this appears to be dead code"
3. **暫不移除**: 給使用者機會提供後端實作到上游
4. **無法編譯**: 選項被完全註解，無法透過正常方式啟用

---

## 🔵 架構 1：obmc_vm (使用 Pipe)

### 編譯選項
```cpp
BMCWEB_VM_WEBSOCKET = true
```

### 程式碼位置
`vm_websocket.hpp:54-190` - `Handler` 類別

### 通訊機制
**Pipe (stdin/stdout)**

### 資料結構
```cpp
class Handler {
    boost::asio::readable_pipe pipeOut;     // 第 178 行
    boost::asio::writable_pipe pipeIn;      // 第 179 行
    boost::process::v2::process proxy;      // 第 180 行

    // Buffer 命名
    boost::beast::flat_static_buffer<nbdBufferSize> outputBuffer;  // 第 183 行
    boost::beast::flat_static_buffer<nbdBufferSize> inputBuffer;   // 第 184 行
};
```

### 完整流程
```
┌──────────┐  WebSocket  ┌─────────┐   Pipe    ┌───────────┐ Unix Socket ┌────────────┐
│ Browser  │ ←─────────→ │ bmcweb  │ ←────────→│ nbd-proxy │←───────────→│ nbd-client │
└──────────┘             └─────────┘  (stdio)   └───────────┘  (.sock)    └────────────┘
                              ↓                       ↓
                         inputBuffer            建立 Unix
                         outputBuffer           Socket
```

### 關鍵程式碼

#### 1. 啟動 nbd-proxy 子進程（第 59-61 行）
```cpp
proxy(ios, "/usr/bin/nbd-proxy", {media},
      boost::process::v2::process_stdio{
          .in = pipeIn, .out = pipeOut, .err = nullptr})
```

#### 2. 寫入到 pipe（第 117 行）
```cpp
pipeIn.async_write_some(
    inputBuffer.data(),
    [this, self(shared_from_this())](const boost::beast::error_code& ec,
                                     std::size_t bytesWritten) { ... });
```

#### 3. 從 pipe 讀取（第 148 行）
```cpp
pipeOut.async_read_some(
    outputBuffer.prepare(bytes),
    [this, self(shared_from_this())](
        const boost::system::error_code& ec, std::size_t bytesRead) { ... });
```

### 責任分工
- **bmcweb**: WebSocket 處理 + 子進程管理
- **nbd-proxy**: Unix Socket 管理 + nbd-client 啟動
- **nbd-client**: NBD 協議處理

---

## 🟢 架構 2：nbd_proxy (使用 Unix Socket)

### 編譯選項
```cpp
BMCWEB_VM_NBDPROXY = true
```

### 程式碼位置
`vm_websocket.hpp:192-536` - `NbdProxyServer` 類別

### 通訊機制
**Unix Domain Socket**

### 資料結構
```cpp
struct NbdProxyServer {
    stream_protocol::socket peerSocket;      // 第 417 行
    stream_protocol::acceptor acceptor;      // 第 420 行

    // Buffer 命名（關鍵！）
    boost::beast::flat_static_buffer<nbdBufferSize> ux2wsBuf;  // 第 411 行 - UNIX → WebSocket
    boost::beast::flat_static_buffer<nbdBufferSize> ws2uxBuf;  // 第 414 行 - WebSocket → UNIX
};
```

### 完整流程
```
┌──────────┐  WebSocket  ┌─────────────────────────────┐ Unix Socket ┌────────────┐
│ Browser  │ ←─────────→ │       bmcweb                │←───────────→│ nbd-client │
└──────────┘             │  (同時是 Unix Socket Server) │  (.sock)    └────────────┘
                         └─────────────────────────────┘
                                     ↓
                              ws2uxBuf / ux2wsBuf

                         ❌ 沒有 nbd-proxy 子進程！
```

### 關鍵程式碼

#### 1. bmcweb 建立 Unix Socket（第 208-209 行）
```cpp
peerSocket(getIoContext()),
acceptor(getIoContext(), stream_protocol::endpoint(socketId))
```

#### 2. 等待 nbd-client 連接（第 289 行）
```cpp
acceptor.async_accept(
    std::bind_front(&NbdProxyServer::afterAccept, weak_from_this()));
```

#### 3. 透過 DBus 請求啟動 nbd-client（第 292-298 行）
```cpp
dbus::utility::async_method_call(
    [weak{weak_from_this()}](const boost::system::error_code& ec,
                             bool isBinary) {
        afterMount(weak, ec, isBinary);
    },
    "xyz.openbmc_project.VirtualMedia", path,
    "xyz.openbmc_project.VirtualMedia.Proxy", "Mount");
```

#### 4. 從 Unix Socket 讀取（第 347 行）
```cpp
peerSocket.async_read_some(ux2wsBuf.prepare(nbdBufferSize),
                           std::bind_front(&NbdProxyServer::afterRead,
                                           this, weak_from_this()));
```

#### 5. 寫入到 Unix Socket（第 397 行）
```cpp
peerSocket.async_write_some(
    ws2uxBuf.data(),
    std::bind_front(&NbdProxyServer::afterWrite, weak_from_this(),
                    std::move(onDone)));
```

### 責任分工
- **bmcweb**: WebSocket 處理 + Unix Socket 伺服器 + DBus 呼叫
- **Virtual Media Service**: nbd-client 啟動與管理
- **nbd-client**: NBD 協議處理

---

## 📊 詳細比較表

| 項目 | 架構 1 (obmc_vm) | 架構 2 (nbd_proxy) |
|------|-----------------|-------------------|
| **狀態** | ✅ **使用中** | ❌ **已棄用 (死代碼)** |
| **編譯選項** | `vm-websocket` | `vm-nbdproxy` |
| **meson.options** | `value: 'enabled'` | **整個選項被註解** |
| **程式碼位置** | 第 54-190 行 | 第 192-536 行 |
| **可編譯** | ✅ 是 | ❌ 否 |
| **後端依賴** | ✅ nbd-proxy (jsnbd 專案) | ❌ 缺少上游實作 |
| **bmcweb 角色** | WebSocket 代理 + 進程管理 | WebSocket 代理 + Unix Socket 伺服器 |
| **nbd-proxy** | ✅ 由 bmcweb 啟動為子進程 | ❌ **不使用** |
| **通訊機制** | Pipe (stdin/stdout) | Unix Domain Socket |
| **Unix Socket 建立者** | nbd-proxy | **bmcweb 本身** |
| **Buffer 命名** | `inputBuffer` / `outputBuffer` | `ws2uxBuf` / `ux2wsBuf` |
| **nbd-client 啟動方式** | 由 nbd-proxy 負責 | Virtual Media service (透過 DBus) |
| **資料複製次數** | 2 次 (WS→Pipe, Pipe→Socket) | 1 次 (WS→Socket) |
| **進程數量** | 3 個 (bmcweb, nbd-proxy, nbd-client) | 2 個 (bmcweb, nbd-client) |

---

## 🔒 編譯時互斥保護

```cpp
// 第 544-545 行
static_assert(
    !(BMCWEB_VM_WEBSOCKET && BMCWEB_VM_NBDPROXY),
    "nbd proxy cannot be turned on at the same time as vm websocket.");
```

這確保了在編譯時只能選擇其中一種架構。

---

## 🔍 Buffer 命名的混淆點

### 問題
`ux2wsBuf` 和 `ws2uxBuf` 這個命名容易讓人誤以為：
- `ux` = Unix Socket (在 bmcweb 和 nbd-proxy 之間)
- 但實際上 bmcweb 和 nbd-proxy 是用 pipe 通訊的（架構 1）

### 真相
這些 buffer **只存在於架構 2**：
- 在架構 2 中，bmcweb 直接透過 Unix Socket 與 nbd-client 通訊
- 所以 `ux` (Unix Socket) ↔ `ws` (WebSocket) 的命名是正確的
- 架構 1 根本不使用這些 buffer

---

## 💡 設計考量

### 架構 1 的優點
- **責任分離**: bmcweb 只負責 WebSocket，nbd-proxy 處理 NBD 細節
- **模組化**: 可以獨立更新 nbd-proxy
- **除錯容易**: 可以單獨測試 nbd-proxy

### 架構 1 的缺點
- **額外進程**: 需要管理 nbd-proxy 子進程
- **額外複製**: 資料需要經過 pipe 複製一次
- **複雜度**: 需要處理子進程生命週期

### 架構 2 的優點
- **效能**: 減少一次資料複製
- **簡化**: 少一個進程要管理
- **直接控制**: bmcweb 直接控制 Unix Socket

### 架構 2 的缺點
- **責任過重**: bmcweb 需要處理更多邏輯
- **耦合度高**: NBD 相關邏輯與 bmcweb 綁定
- **測試困難**: 無法獨立測試 Unix Socket 部分

---

## 📝 相關檔案

### bmcweb 配置
- `bmcweb/meson_options.txt` - 定義 `vm-websocket` 和 `vm-nbdproxy` 選項

### nbd-proxy 實作
- `jsnbd/nbd-proxy.c` - 架構 1 使用的外部工具

### Virtual Media Service
- `phosphor-dbus-interfaces/yaml/xyz/openbmc_project/VirtualMedia/Proxy.interface.yaml`
- DBus 介面定義 `Mount()` 和 `Unmount()` 方法

---

## 🎯 結論

### ✅ 實際使用的架構
**只有架構 1 (obmc_vm) 在生產環境中使用：**
- 編譯選項: `vm-websocket` (預設啟用)
- 流程: bmcweb ↔ pipe ↔ nbd-proxy ↔ Unix Socket ↔ nbd-client
- 依賴: jsnbd 專案的 nbd-proxy 工具
- 狀態: ✅ 完全支援，有上游實作

### ❌ 已棄用的架構
**架構 2 (nbd_proxy) 是死代碼：**
- 編譯選項: `vm-nbdproxy` (整個選項被註解)
- 原因: 缺少必要的後端 daemon 實作
- 狀態: ❌ 無法編譯，等待移除或有人提供實作
- 參考: https://github.com/openbmc/bmcweb/issues/188

### 📝 關鍵發現
1. **兩種架構設計互斥**: 但實際上只有一種能用
2. **Buffer 命名混淆澄清**:
   - `ux2wsBuf` / `ws2uxBuf` 只存在於死代碼中
   - 實際使用的是 `inputBuffer` / `outputBuffer`
3. **架構 2 的理論優勢**: 效能更好（少一層抽象），但缺乏實作支援
4. **保留原因**: 給私有部署機會貢獻後端實作到上游

---

## 🔬 後續研究方向

1. ✅ ~~檢查預設使用哪種架構~~ - **已確認：架構 1**
2. 深入研究 nbd-proxy.c 的實作細節
3. 探討架構 2 為何會失敗（缺少什麼樣的 daemon？）
4. 研究架構演進歷史（commit history）
5. 追蹤 GitHub Issue #188 的討論（若有權限）

---

## 參考資料

- `bmcweb/features/virtual_media/vm_websocket.hpp`
- `jsnbd/nbd-proxy.c`
- NBD Protocol: https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md
