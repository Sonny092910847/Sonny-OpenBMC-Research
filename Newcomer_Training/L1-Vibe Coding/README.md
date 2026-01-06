# L1-Vibe Coding

## 概述
使用 Claude Code (Vibe Coding) 完成的 L1 新人訓練作業。

## 完成項目

| 項目 | 狀態 | 說明 |
|------|------|------|
| Written Exam | ✓ | 100/100 分（使用 auto_quiz.py 自動答題）|
| Exercise 1 - C++ | ✓ | 輸出 `BMC L1 Training - ID : 835051, Name : Sonny Chu` |
| Exercise 1 - Systemd | ✓ | `bmc.L1.id835051.service` enabled 且開機執行 |
| Exercise 2 - Git | ✓ | add, commit, checkout, stash 演示完成 |
| Exercise 3 - IPMI | ✓ | Out-of-band SDR 查詢成功 |

## 目錄結構

```
L1-Vibe Coding/
├── README.md                    # 本檔案
├── auto_quiz.py                 # 自動答題腳本
├── backups/
│   ├── CHANGELOG.md             # 完整開發記錄
│   └── diffs/                   # 新增檔案備份
│       ├── layer.conf.新增
│       ├── bmc-l1-training.bb.新增
│       ├── bmc_L1_ID835051.cpp.新增
│       ├── bmc.L1.id835051.service.新增
│       ├── intel-platforms.bbappend.新增
│       └── auto_quiz.py.新增
└── meta-bmc-training/           # BitBake meta layer
    ├── conf/
    │   └── layer.conf
    ├── recipes-training/
    │   └── bmc-l1-training/
    │       ├── bmc-l1-training.bb
    │       └── files/
    │           ├── bmc_L1_ID835051.cpp
    │           └── bmc.L1.id835051.service
    └── recipes-intel/
        └── images/
            └── intel-platforms.bbappend
```

## 使用方式

### 1. 複製 meta-bmc-training 到 BHS 環境
```bash
cp -r meta-bmc-training ~/bhs-openbmc-project/bhs-openbmc/
```

### 2. 加入 layer 並編譯
```bash
cd ~/bhs-openbmc-project/bhs-openbmc && source oe-init-build-env
bitbake-layers add-layer ../meta-bmc-training
bitbake intel-platforms
```

### 3. QEMU 測試
```bash
# 啟動 QEMU
/path/to/qemu-system-arm -m 1G -M intel-ast2600 -nographic \
  -drive file=image-mtd,format=raw,if=mtd \
  -net nic -net user,hostfwd=:127.0.0.1:2228-:22

# SSH 連線
ssh -p 2228 root@127.0.0.1  # 密碼: 0penBmc

# 驗證
/usr/bin/bmc_L1_ID835051
systemctl status bmc.L1.id835051.service
```

## 重要配置

### layer.conf 關鍵設定
```bash
LAYERDEPENDS_meta-bmc-training = "core phosphor-layer"  # 不是 meta-phosphor
LAYERSERIES_COMPAT_meta-bmc-training = "mickledore"     # 不是 nanbield
```

## 作者
- 訓練人員：Sonny Chu
- 工號：835051
- 日期：2026-01-06
- 工具：Claude Code (Vibe Coding)
