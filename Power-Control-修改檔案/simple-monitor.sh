#!/bin/bash

###############################################################################
# 簡化版 Power Control 監控腳本
# 使用方法: ssh root@<BMC_IP> "bash -s" < simple-monitor.sh
###############################################################################

echo "=========================================="
echo "🔍 開始監控 Power Control 相關服務"
echo "=========================================="
echo ""

# 同時監控 bmcweb 和 State Manager
journalctl -u bmcweb -u xyz.openbmc_project.State.Host -f --no-pager \
    | grep --line-buffered -E "POST|Reset|ResetType|Transition|RequestedHostTransition|CurrentHostState|D-Bus"
