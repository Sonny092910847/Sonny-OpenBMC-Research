# L2: D-Bus Service Implementation

## 檔案說明

| 檔案 | 說明 |
|------|------|
| `sonnychu-l2-service_1.0.bb` | BitBake recipe |
| `files/sonnychu-l2-service.cpp` | D-Bus Service 主程式（C++23） |
| `files/meson.build` | Meson 編譯設定 |
| `files/sonnychu-l2-service.service` | systemd service 檔案 |

## 使用方式

### 1. 建立 meta layer 結構
```bash
mkdir -p ~/openbmc/meta-bmc-training/recipes-l2/sonnychu-l2-service/files
cp sonnychu-l2-service_1.0.bb ~/openbmc/meta-bmc-training/recipes-l2/sonnychu-l2-service/
cp files/* ~/openbmc/meta-bmc-training/recipes-l2/sonnychu-l2-service/files/
```

### 2. 建立 layer.conf
```bash
mkdir -p ~/openbmc/meta-bmc-training/conf
cat > ~/openbmc/meta-bmc-training/conf/layer.conf << 'CONF'
BBPATH .= ":${LAYERDIR}"
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb"
BBFILE_COLLECTIONS += "meta-bmc-training"
BBFILE_PATTERN_meta-bmc-training = "^${LAYERDIR}/"
LAYERSERIES_COMPAT_meta-bmc-training = "nanbield"
CONF
```

### 3. 加入 bblayers.conf

在 `~/openbmc/build/romulus/conf/bblayers.conf` 加入：
```
BBLAYERS += "${TOPDIR}/../meta-bmc-training"
```

### 4. 加入 image

建立 `meta-bmc-training/recipes-phosphor/images/obmc-phosphor-image.bbappend`：
```
IMAGE_INSTALL += "sonnychu-l2-service"
```

### 5. 編譯
```bash
bitbake obmc-phosphor-image
```

## D-Bus 資訊

| 項目 | 值 |
|------|-----|
| Service | `com.foxconn.SonnyChu.L2Service` |
| Object Path | `/com/foxconn/sonnychu/l2service` |
| Interface | `com.foxconn.SonnyChu.L2Interface` |
| Properties | Level (uint32), Message (string) |

## 功能說明

- **Level 1**: 基本 D-Bus Service，提供 Level 和 Message 兩個 Property
- **Level 2**: 自動監控 `/tmp/test/test_value` 檔案，更新 Level 值
- **Level 3**: 當 Level ≥ 80 時發送 Alarm signal，恢復時發送 Normal signal
