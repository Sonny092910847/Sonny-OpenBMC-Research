#!/usr/bin/env python3
"""
QEMU AST2600 iKVM 不可行性驗證腳本

此腳本透過實際檢查系統狀態來驗證 iKVM 無法在 QEMU AST2600 上執行的證據。
適用於在 QEMU 模擬的 OpenBMC 環境中執行。

作者: Claude AI
日期: 2025-11-12
版本: 1.0
"""

import os
import sys
import subprocess
import json
from pathlib import Path
from typing import Dict, List, Tuple
from datetime import datetime


class IKVMImpossibilityVerifier:
    """驗證 iKVM 在 QEMU AST2600 上不可行的證據收集器"""

    def __init__(self):
        self.results = {
            "timestamp": datetime.now().isoformat(),
            "platform": self._detect_platform(),
            "evidence": {},
            "summary": {
                "total_checks": 0,
                "failures": 0,
                "expected_failures": 0
            }
        }

    def _detect_platform(self) -> str:
        """檢測目前執行的平台"""
        try:
            with open("/proc/device-tree/model", "r") as f:
                model = f.read().strip()
                if "aspeed" in model.lower():
                    if "ast2600" in model.lower():
                        return "AST2600 (可能是 QEMU)"
                    elif "ast2500" in model.lower():
                        return "AST2500 (可能是 QEMU)"
                return model
        except FileNotFoundError:
            return "未知平台"

    def _run_command(self, cmd: List[str], timeout: int = 5) -> Tuple[int, str, str]:
        """執行 shell 命令並回傳結果"""
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "命令逾時"
        except Exception as e:
            return -2, "", str(e)

    def check_video_device(self) -> Dict:
        """證據1: 檢查 /dev/video0 裝置是否存在"""
        self.results["summary"]["total_checks"] += 1

        video_device = Path("/dev/video0")
        exists = video_device.exists()

        # 在 QEMU 上，我們期望這個裝置不存在
        is_expected_failure = not exists

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "Video 裝置存在性檢查",
            "device_path": "/dev/video0",
            "exists": exists,
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 證據確認：裝置不存在（預期）" if is_expected_failure else "✗ 意外：裝置存在",
            "explanation": "obmc-ikvm 需要 /dev/video0 裝置。在 QEMU 中由於 Video Engine 是 TYPE_UNIMPLEMENTED_DEVICE，驅動程式無法建立此裝置。"
        }

        # 如果裝置存在，檢查其屬性
        if exists:
            try:
                stat_info = video_device.stat()
                evidence["device_info"] = {
                    "mode": oct(stat_info.st_mode),
                    "major": os.major(stat_info.st_rdev),
                    "minor": os.minor(stat_info.st_rdev)
                }
            except Exception as e:
                evidence["error"] = str(e)

        self.results["evidence"]["video_device"] = evidence
        return evidence

    def check_hidg_devices(self) -> Dict:
        """證據2: 檢查 USB HID gadget 裝置是否存在"""
        self.results["summary"]["total_checks"] += 1

        hidg_devices = ["/dev/hidg0", "/dev/hidg1"]
        device_status = {}
        all_missing = True

        for device_path in hidg_devices:
            device = Path(device_path)
            exists = device.exists()
            device_status[device_path] = exists
            if exists:
                all_missing = False

        # 在 QEMU 上，我們期望這些裝置不存在
        is_expected_failure = all_missing

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "USB HID Gadget 裝置存在性檢查",
            "devices": device_status,
            "all_missing": all_missing,
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 證據確認：HID 裝置不存在（預期）" if is_expected_failure else "✗ 意外：部分/全部裝置存在",
            "explanation": "obmc-ikvm 需要 /dev/hidg0 和 /dev/hidg1 裝置。在 QEMU 中由於 USB Device Controller 是 UnimplementedDeviceState，無法建立 USB gadget 裝置。"
        }

        self.results["evidence"]["hidg_devices"] = evidence
        return evidence

    def check_aspeed_video_driver(self) -> Dict:
        """證據3: 檢查 aspeed-video 驅動程式狀態"""
        self.results["summary"]["total_checks"] += 1

        # 檢查驅動程式是否載入
        returncode, stdout, stderr = self._run_command(["lsmod"])
        driver_loaded = "aspeed_video" in stdout or "aspeed-video" in stdout

        # 檢查 dmesg 中的錯誤訊息
        returncode, dmesg_output, _ = self._run_command(
            ["dmesg"], timeout=10
        )

        timeout_errors = []
        if dmesg_output:
            for line in dmesg_output.split("\n"):
                if "aspeed-video" in line or "aspeed_video" in line:
                    if any(keyword in line.lower() for keyword in
                           ["timeout", "timed out", "failed", "error"]):
                        timeout_errors.append(line.strip())

        has_timeout_errors = len(timeout_errors) > 0

        # 在 QEMU 上，我們期望看到逾時錯誤
        is_expected_failure = has_timeout_errors

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "aspeed-video 驅動程式狀態檢查",
            "driver_loaded": driver_loaded,
            "timeout_errors_found": has_timeout_errors,
            "error_count": len(timeout_errors),
            "sample_errors": timeout_errors[:5],  # 只顯示前5個錯誤
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 證據確認：驅動程式逾時錯誤（預期）" if is_expected_failure else "✗ 意外：驅動程式正常工作",
            "explanation": "在 QEMU 中，aspeed-video 驅動程式嘗試初始化時會遇到逾時，因為底層硬體不回應暫存器存取。"
        }

        self.results["evidence"]["aspeed_video_driver"] = evidence
        return evidence

    def check_ikvm_service(self) -> Dict:
        """證據4: 檢查 obmc-ikvm 服務狀態"""
        self.results["summary"]["total_checks"] += 1

        service_names = ["start-ipkvm.service", "obmc-ikvm.service"]
        service_status = {}

        for service_name in service_names:
            returncode, stdout, stderr = self._run_command(
                ["systemctl", "is-active", service_name]
            )

            is_active = stdout.strip() == "active"
            service_status[service_name] = {
                "active": is_active,
                "status": stdout.strip()
            }

            # 取得服務詳細狀態
            returncode, status_output, _ = self._run_command(
                ["systemctl", "status", service_name]
            )

            # 尋找錯誤訊息
            error_messages = []
            if status_output:
                for line in status_output.split("\n"):
                    if any(keyword in line.lower() for keyword in
                           ["failed", "error", "no such file"]):
                        error_messages.append(line.strip())

            service_status[service_name]["error_messages"] = error_messages

        any_active = any(s["active"] for s in service_status.values())

        # 在 QEMU 上，我們期望服務未執行
        is_expected_failure = not any_active

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "obmc-ikvm 服務狀態檢查",
            "services": service_status,
            "any_service_active": any_active,
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 證據確認：服務未執行（預期）" if is_expected_failure else "✗ 意外：服務正在執行",
            "explanation": "obmc-ikvm 服務由於無法開啟 /dev/video0 和 /dev/hidg0 而啟動失敗。"
        }

        self.results["evidence"]["ikvm_service"] = evidence
        return evidence

    def check_vnc_port(self) -> Dict:
        """證據5: 檢查 VNC 埠 5900 是否在監聽"""
        self.results["summary"]["total_checks"] += 1

        # 檢查埠 5900 是否開放
        returncode, stdout, stderr = self._run_command(
            ["ss", "-tlnp"]
        )

        port_listening = False
        vnc_process = None

        if stdout:
            for line in stdout.split("\n"):
                if ":5900" in line:
                    port_listening = True
                    vnc_process = line.strip()
                    break

        # 在 QEMU 上，我們期望 VNC 埠未監聽
        is_expected_failure = not port_listening

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "VNC 埠監聽檢查",
            "port": 5900,
            "listening": port_listening,
            "process_info": vnc_process if vnc_process else "無程序監聽",
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 證據確認：VNC 埠未監聽（預期）" if is_expected_failure else "✗ 意外：VNC 埠正在監聽",
            "explanation": "bmcweb 需要連線到 127.0.0.1:5900 的 obmc-ikvm VNC 伺服器。由於 obmc-ikvm 無法啟動，此埠不會被監聽。"
        }

        self.results["evidence"]["vnc_port"] = evidence
        return evidence

    def check_kernel_modules(self) -> Dict:
        """證據6: 檢查相關核心模組"""
        self.results["summary"]["total_checks"] += 1

        modules_to_check = [
            "aspeed_video",
            "aspeed_vhub",
            "aspeed_udc",
            "videodev",
            "v4l2_common"
        ]

        returncode, stdout, stderr = self._run_command(["lsmod"])

        module_status = {}
        for module in modules_to_check:
            loaded = module in stdout
            module_status[module] = {
                "loaded": loaded,
                "expected_in_qemu": module in ["videodev", "v4l2_common"]  # 這些可能被載入但無用
            }

        evidence = {
            "test_name": "核心模組載入狀態",
            "modules": module_status,
            "explanation": "即使某些 V4L2 相關模組被載入，如果 aspeed_video 驅動程式初始化失敗，裝置檔案也不會被建立。"
        }

        self.results["evidence"]["kernel_modules"] = evidence
        return evidence

    def check_device_tree(self) -> Dict:
        """證據7: 檢查裝置樹設定"""
        self.results["summary"]["total_checks"] += 1

        dt_paths = [
            "/proc/device-tree/ahb/video/compatible",
            "/proc/device-tree/ahb/usb-vhub@1e6a0000/compatible",
            "/sys/firmware/devicetree/base/ahb/video/compatible",
        ]

        dt_info = {}
        for path in dt_paths:
            p = Path(path)
            if p.exists():
                try:
                    with open(p, "r") as f:
                        content = f.read().strip('\0')
                        dt_info[path] = {
                            "exists": True,
                            "content": content
                        }
                except Exception as e:
                    dt_info[path] = {
                        "exists": True,
                        "error": str(e)
                    }
            else:
                dt_info[path] = {"exists": False}

        evidence = {
            "test_name": "裝置樹設定檢查",
            "device_tree_nodes": dt_info,
            "explanation": "裝置樹節點可能存在，但在 QEMU 中對應的硬體是 TYPE_UNIMPLEMENTED_DEVICE，不會回應驅動程式存取。"
        }

        self.results["evidence"]["device_tree"] = evidence
        return evidence

    def check_qemu_detection(self) -> Dict:
        """證據8: 嘗試偵測是否執行在 QEMU 中"""
        self.results["summary"]["total_checks"] += 1

        qemu_indicators = {
            "dmesg_qemu": False,
            "cpuinfo_qemu": False,
            "platform_qemu": False
        }

        # 檢查 dmesg
        returncode, stdout, _ = self._run_command(["dmesg"], timeout=10)
        if stdout and "qemu" in stdout.lower():
            qemu_indicators["dmesg_qemu"] = True

        # 檢查 cpuinfo
        try:
            with open("/proc/cpuinfo", "r") as f:
                cpuinfo = f.read()
                if "qemu" in cpuinfo.lower():
                    qemu_indicators["cpuinfo_qemu"] = True
        except:
            pass

        # 檢查平台資訊
        if "qemu" in self.results["platform"].lower():
            qemu_indicators["platform_qemu"] = True

        is_qemu = any(qemu_indicators.values())

        evidence = {
            "test_name": "QEMU 環境偵測",
            "indicators": qemu_indicators,
            "is_likely_qemu": is_qemu,
            "confidence": "高" if sum(qemu_indicators.values()) >= 2 else "中" if any(qemu_indicators.values()) else "低",
            "explanation": "如果偵測到 QEMU 環境，所有 iKVM 相關的失敗都是預期的。"
        }

        self.results["evidence"]["qemu_detection"] = evidence
        return evidence

    def generate_report(self) -> str:
        """生成人類可讀的報告"""
        report_lines = []
        report_lines.append("=" * 80)
        report_lines.append("QEMU AST2600 iKVM 不可行性驗證報告")
        report_lines.append("=" * 80)
        report_lines.append(f"\n時間戳記: {self.results['timestamp']}")
        report_lines.append(f"平台: {self.results['platform']}")
        report_lines.append(f"\n檢查項總數: {self.results['summary']['total_checks']}")
        report_lines.append(f"失敗項: {self.results['summary']['failures']}")
        report_lines.append(f"預期失敗項（QEMU 限制）: {self.results['summary']['expected_failures']}")

        # 計算置信度
        if self.results['summary']['expected_failures'] >= 4:
            confidence = "非常高"
            conclusion = "✓ 證據充分：iKVM 在 QEMU AST2600 上不可行"
        elif self.results['summary']['expected_failures'] >= 2:
            confidence = "高"
            conclusion = "✓ 證據較強：iKVM 在 QEMU AST2600 上不可行"
        else:
            confidence = "低"
            conclusion = "⚠ 證據不足或環境可能不是 QEMU"

        report_lines.append(f"\n置信度: {confidence}")
        report_lines.append(f"結論: {conclusion}")

        report_lines.append("\n" + "=" * 80)
        report_lines.append("詳細證據")
        report_lines.append("=" * 80)

        for key, evidence in self.results["evidence"].items():
            report_lines.append(f"\n【{evidence.get('test_name', key)}】")
            if 'verdict' in evidence:
                report_lines.append(f"  判定: {evidence['verdict']}")
            if 'explanation' in evidence:
                report_lines.append(f"  說明: {evidence['explanation']}")

            # 顯示關鍵細節
            if key == "video_device":
                report_lines.append(f"  裝置存在: {evidence['exists']}")
            elif key == "hidg_devices":
                report_lines.append(f"  裝置狀態: {evidence['devices']}")
            elif key == "aspeed_video_driver":
                report_lines.append(f"  驅動程式已載入: {evidence['driver_loaded']}")
                report_lines.append(f"  逾時錯誤: {evidence['timeout_errors_found']}")
                if evidence['sample_errors']:
                    report_lines.append(f"  範例錯誤: {evidence['sample_errors'][0][:120]}...")
            elif key == "ikvm_service":
                for svc, status in evidence['services'].items():
                    report_lines.append(f"  {svc}: {status['status']}")
            elif key == "vnc_port":
                report_lines.append(f"  埠 5900 監聽: {evidence['listening']}")
            elif key == "qemu_detection":
                report_lines.append(f"  可能是 QEMU: {evidence['is_likely_qemu']}")
                report_lines.append(f"  置信度: {evidence['confidence']}")

        report_lines.append("\n" + "=" * 80)
        report_lines.append("證明邏輯鏈")
        report_lines.append("=" * 80)
        report_lines.append("""
1. QEMU AST2600 將 Video Engine 標記為 TYPE_UNIMPLEMENTED_DEVICE
   ↓
2. aspeed-video 驅動程式初始化時遇到逾時（硬體不回應）
   ↓
3. /dev/video0 裝置檔案未被建立
   ↓
4. obmc-ikvm 無法開啟 /dev/video0，啟動失敗
   ↓
5. VNC 伺服器未在 5900 埠監聽
   ↓
6. bmcweb 無法連線到 127.0.0.1:5900
   ↓
7. iKVM 功能完全不可用

同理，USB Device Controller (UDC) 也是 UnimplementedDeviceState：
- 無 USB 裝置控制器硬體 → 無 USB gadget 框架 → 無 /dev/hidg0 → obmc-ikvm 失敗
        """)

        report_lines.append("\n" + "=" * 80)
        report_lines.append("原始碼證據位置")
        report_lines.append("=" * 80)
        report_lines.append("""
QEMU 原始碼:
- hw/arm/aspeed_ast2600.c:130 (約)
  object_initialize_child(obj, "video", &s->video, TYPE_UNIMPLEMENTED_DEVICE);

- include/hw/arm/aspeed_soc.h:70 (約)
  UnimplementedDeviceState video;
  UnimplementedDeviceState udc;

Linux 核心:
- drivers/media/platform/aspeed/aspeed-video.c
  aspeed_video_probe() 和逾時處理

OpenBMC:
- github.com/openbmc/obmc-ikvm
  ikvm_video.cpp: Video::start() - open("/dev/video0")
  ikvm_input.cpp: Input::Input() - open("/dev/hidg0")

詳細文件:
- 請查看 QEMU_AST2600_IKVM_IMPOSSIBILITY_PROOF.md
        """)

        report_lines.append("\n" + "=" * 80)
        report_lines.append("報告結束")
        report_lines.append("=" * 80)

        return "\n".join(report_lines)

    def run_all_checks(self):
        """執行所有檢查"""
        print("開始收集證據...\n")

        checks = [
            ("偵測 QEMU 環境", self.check_qemu_detection),
            ("檢查 Video 裝置", self.check_video_device),
            ("檢查 HID Gadget 裝置", self.check_hidg_devices),
            ("檢查 aspeed-video 驅動程式", self.check_aspeed_video_driver),
            ("檢查 obmc-ikvm 服務", self.check_ikvm_service),
            ("檢查 VNC 埠", self.check_vnc_port),
            ("檢查核心模組", self.check_kernel_modules),
            ("檢查裝置樹", self.check_device_tree),
        ]

        for name, check_func in checks:
            print(f"正在執行: {name}...", end=" ")
            try:
                result = check_func()
                if result.get("verdict"):
                    status = "✓" if "證據確認" in result["verdict"] else "✗"
                    print(f"{status}")
                else:
                    print("完成")
            except Exception as e:
                print(f"錯誤: {e}")

        print("\n證據收集完成！\n")

    def save_json_report(self, filename: str = "ikvm_impossibility_evidence.json"):
        """儲存 JSON 格式的報告"""
        with open(filename, "w", encoding="utf-8") as f:
            json.dump(self.results, f, indent=2, ensure_ascii=False)
        print(f"JSON 報告已儲存到: {filename}")

    def save_text_report(self, filename: str = "ikvm_impossibility_report.txt"):
        """儲存文字格式的報告"""
        report = self.generate_report()
        with open(filename, "w", encoding="utf-8") as f:
            f.write(report)
        print(f"文字報告已儲存到: {filename}")


def main():
    print("""
╔════════════════════════════════════════════════════════════════════════════╗
║           QEMU AST2600 iKVM 不可行性自動驗證工具                           ║
║                                                                            ║
║  此工具將收集證據證明 iKVM 無法在 QEMU AST2600 環境中執行                 ║
║  建議在 QEMU 模擬的 OpenBMC 環境中執行以獲得最佳結果                       ║
╚════════════════════════════════════════════════════════════════════════════╝
    """)

    # 檢查是否以 root 執行（某些檢查可能需要）
    if os.geteuid() != 0:
        print("警告: 未以 root 權限執行，某些檢查可能無法完成。")
        print("建議使用: sudo python3 verify_ikvm_impossibility.py\n")

    verifier = IKVMImpossibilityVerifier()

    try:
        # 執行所有檢查
        verifier.run_all_checks()

        # 生成並顯示報告
        report = verifier.generate_report()
        print(report)

        # 儲存報告
        verifier.save_text_report()
        verifier.save_json_report()

        print("\n" + "=" * 80)
        print("驗證完成！")
        print("=" * 80)

        # 回傳適當的退出碼
        if verifier.results["summary"]["expected_failures"] >= 4:
            print("\n✓ 證據充分：已證明 iKVM 在 QEMU AST2600 上不可行")
            return 0
        else:
            print("\n⚠ 警告：證據不足或環境可能不是 QEMU")
            return 1

    except KeyboardInterrupt:
        print("\n\n使用者中斷，退出...")
        return 130
    except Exception as e:
        print(f"\n錯誤: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
