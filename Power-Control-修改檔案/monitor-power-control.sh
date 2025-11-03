#!/bin/bash

###############################################################################
# Power Control 完整監控腳本
# 用途: 同時監控 bmcweb、D-Bus、和 State Manager 的所有訊息
# 使用方法:
#   1. 上傳到 BMC: scp monitor-power-control.sh root@<BMC_IP>:/tmp/
#   2. SSH 連線: ssh root@<BMC_IP>
#   3. 執行: bash /tmp/monitor-power-control.sh
###############################################################################

echo "=========================================="
echo "🔍 Power Control 監控腳本啟動"
echo "=========================================="
echo ""
echo "此腳本會同時監控以下服務:"
echo "  1. bmcweb (Redfish API Server)"
echo "  2. D-Bus 系統訊息 (State.Host 介面)"
echo "  3. phosphor-state-manager (Host State Manager)"
echo ""
echo "現在開始監控... (按 Ctrl+C 停止)"
echo "=========================================="
echo ""

# 創建命名管道 (named pipes) 來合併多個日誌輸出
PIPE_DIR="/tmp/power-monitor-$$"
mkdir -p "$PIPE_DIR"

BMCWEB_PIPE="$PIPE_DIR/bmcweb"
DBUS_PIPE="$PIPE_DIR/dbus"
STATE_PIPE="$PIPE_DIR/state"

mkfifo "$BMCWEB_PIPE" "$DBUS_PIPE" "$STATE_PIPE"

# 清理函數
cleanup() {
    echo ""
    echo "=========================================="
    echo "🛑 監控已停止"
    echo "=========================================="
    rm -rf "$PIPE_DIR"
    kill 0  # 殺掉所有子進程
}

trap cleanup EXIT INT TERM

# 啟動 bmcweb 日誌監控（背景執行）
(
    journalctl -u bmcweb -f --no-pager 2>/dev/null | while IFS= read -r line; do
        echo "🌐 [bmcweb] $line"
    done > "$BMCWEB_PIPE"
) &

# 啟動 D-Bus 監控（背景執行）
(
    dbus-monitor --system "interface='xyz.openbmc_project.State.Host'" 2>&1 | while IFS= read -r line; do
        # 只顯示重要的 D-Bus 訊息
        if [[ "$line" =~ "method call" ]] || \
           [[ "$line" =~ "method return" ]] || \
           [[ "$line" =~ "signal" ]] || \
           [[ "$line" =~ "RequestedHostTransition" ]] || \
           [[ "$line" =~ "CurrentHostState" ]] || \
           [[ "$line" =~ "Transition.On" ]] || \
           [[ "$line" =~ "Transition.Off" ]]; then
            echo "🔌 [D-Bus] $line"
        fi
    done > "$DBUS_PIPE"
) &

# 啟動 State Manager 日誌監控（背景執行）
(
    journalctl -u xyz.openbmc_project.State.Host -f --no-pager 2>/dev/null | while IFS= read -r line; do
        echo "⚙️  [State Manager] $line"
    done > "$STATE_PIPE"
) &

# 合併所有管道的輸出並顯示
tail -f "$BMCWEB_PIPE" "$DBUS_PIPE" "$STATE_PIPE" 2>/dev/null

# 等待所有背景進程
wait
