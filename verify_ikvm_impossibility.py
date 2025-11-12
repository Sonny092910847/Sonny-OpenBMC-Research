#!/usr/bin/env python3
"""
QEMU AST2600 iKVM不可行性验证脚本

此脚本通过实际检查系统状态来验证iKVM无法在QEMU AST2600上运行的证据。
适用于在QEMU模拟的OpenBMC环境中运行。

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
    """验证iKVM在QEMU AST2600上不可行的证据收集器"""

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
        """检测当前运行的平台"""
        try:
            with open("/proc/device-tree/model", "r") as f:
                model = f.read().strip()
                if "aspeed" in model.lower():
                    if "ast2600" in model.lower():
                        return "AST2600 (可能是QEMU)"
                    elif "ast2500" in model.lower():
                        return "AST2500 (可能是QEMU)"
                return model
        except FileNotFoundError:
            return "未知平台"

    def _run_command(self, cmd: List[str], timeout: int = 5) -> Tuple[int, str, str]:
        """运行shell命令并返回结果"""
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "命令超时"
        except Exception as e:
            return -2, "", str(e)

    def check_video_device(self) -> Dict:
        """证据1: 检查/dev/video0设备是否存在"""
        self.results["summary"]["total_checks"] += 1

        video_device = Path("/dev/video0")
        exists = video_device.exists()

        # 在QEMU上，我们期望这个设备不存在
        is_expected_failure = not exists

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "Video设备存在性检查",
            "device_path": "/dev/video0",
            "exists": exists,
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 证据确认：设备不存在（预期）" if is_expected_failure else "✗ 意外：设备存在",
            "explanation": "obmc-ikvm需要/dev/video0设备。在QEMU中由于Video Engine是TYPE_UNIMPLEMENTED_DEVICE，驱动无法创建此设备。"
        }

        # 如果设备存在，检查其属性
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
        """证据2: 检查USB HID gadget设备是否存在"""
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

        # 在QEMU上，我们期望这些设备不存在
        is_expected_failure = all_missing

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "USB HID Gadget设备存在性检查",
            "devices": device_status,
            "all_missing": all_missing,
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 证据确认：HID设备不存在（预期）" if is_expected_failure else "✗ 意外：部分/全部设备存在",
            "explanation": "obmc-ikvm需要/dev/hidg0和/dev/hidg1设备。在QEMU中由于USB Device Controller是UnimplementedDeviceState，无法创建USB gadget设备。"
        }

        self.results["evidence"]["hidg_devices"] = evidence
        return evidence

    def check_aspeed_video_driver(self) -> Dict:
        """证据3: 检查aspeed-video驱动状态"""
        self.results["summary"]["total_checks"] += 1

        # 检查驱动是否加载
        returncode, stdout, stderr = self._run_command(["lsmod"])
        driver_loaded = "aspeed_video" in stdout or "aspeed-video" in stdout

        # 检查dmesg中的错误消息
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

        # 在QEMU上，我们期望看到超时错误
        is_expected_failure = has_timeout_errors

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "aspeed-video驱动状态检查",
            "driver_loaded": driver_loaded,
            "timeout_errors_found": has_timeout_errors,
            "error_count": len(timeout_errors),
            "sample_errors": timeout_errors[:5],  # 只显示前5个错误
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 证据确认：驱动超时错误（预期）" if is_expected_failure else "✗ 意外：驱动正常工作",
            "explanation": "在QEMU中，aspeed-video驱动尝试初始化时会遇到超时，因为底层硬件不响应寄存器访问。"
        }

        self.results["evidence"]["aspeed_video_driver"] = evidence
        return evidence

    def check_ikvm_service(self) -> Dict:
        """证据4: 检查obmc-ikvm服务状态"""
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

            # 获取服务详细状态
            returncode, status_output, _ = self._run_command(
                ["systemctl", "status", service_name]
            )

            # 查找错误消息
            error_messages = []
            if status_output:
                for line in status_output.split("\n"):
                    if any(keyword in line.lower() for keyword in
                           ["failed", "error", "no such file"]):
                        error_messages.append(line.strip())

            service_status[service_name]["error_messages"] = error_messages

        any_active = any(s["active"] for s in service_status.values())

        # 在QEMU上，我们期望服务未运行
        is_expected_failure = not any_active

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "obmc-ikvm服务状态检查",
            "services": service_status,
            "any_service_active": any_active,
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 证据确认：服务未运行（预期）" if is_expected_failure else "✗ 意外：服务正在运行",
            "explanation": "obmc-ikvm服务由于无法打开/dev/video0和/dev/hidg0而启动失败。"
        }

        self.results["evidence"]["ikvm_service"] = evidence
        return evidence

    def check_vnc_port(self) -> Dict:
        """证据5: 检查VNC端口5900是否在监听"""
        self.results["summary"]["total_checks"] += 1

        # 检查端口5900是否开放
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

        # 在QEMU上，我们期望VNC端口未监听
        is_expected_failure = not port_listening

        if is_expected_failure:
            self.results["summary"]["expected_failures"] += 1
            self.results["summary"]["failures"] += 1

        evidence = {
            "test_name": "VNC端口监听检查",
            "port": 5900,
            "listening": port_listening,
            "process_info": vnc_process if vnc_process else "无进程监听",
            "is_qemu_limitation": is_expected_failure,
            "verdict": "✓ 证据确认：VNC端口未监听（预期）" if is_expected_failure else "✗ 意外：VNC端口正在监听",
            "explanation": "bmcweb需要连接到127.0.0.1:5900的obmc-ikvm VNC服务器。由于obmc-ikvm无法启动，此端口不会被监听。"
        }

        self.results["evidence"]["vnc_port"] = evidence
        return evidence

    def check_kernel_modules(self) -> Dict:
        """证据6: 检查相关内核模块"""
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
                "expected_in_qemu": module in ["videodev", "v4l2_common"]  # 这些可能被加载但无用
            }

        evidence = {
            "test_name": "内核模块加载状态",
            "modules": module_status,
            "explanation": "即使某些V4L2相关模块被加载，如果aspeed_video驱动初始化失败，设备文件也不会被创建。"
        }

        self.results["evidence"]["kernel_modules"] = evidence
        return evidence

    def check_device_tree(self) -> Dict:
        """证据7: 检查设备树配置"""
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
            "test_name": "设备树配置检查",
            "device_tree_nodes": dt_info,
            "explanation": "设备树节点可能存在，但在QEMU中对应的硬件是TYPE_UNIMPLEMENTED_DEVICE，不会响应驱动访问。"
        }

        self.results["evidence"]["device_tree"] = evidence
        return evidence

    def check_qemu_detection(self) -> Dict:
        """证据8: 尝试检测是否运行在QEMU中"""
        self.results["summary"]["total_checks"] += 1

        qemu_indicators = {
            "dmesg_qemu": False,
            "cpuinfo_qemu": False,
            "platform_qemu": False
        }

        # 检查dmesg
        returncode, stdout, _ = self._run_command(["dmesg"], timeout=10)
        if stdout and "qemu" in stdout.lower():
            qemu_indicators["dmesg_qemu"] = True

        # 检查cpuinfo
        try:
            with open("/proc/cpuinfo", "r") as f:
                cpuinfo = f.read()
                if "qemu" in cpuinfo.lower():
                    qemu_indicators["cpuinfo_qemu"] = True
        except:
            pass

        # 检查平台信息
        if "qemu" in self.results["platform"].lower():
            qemu_indicators["platform_qemu"] = True

        is_qemu = any(qemu_indicators.values())

        evidence = {
            "test_name": "QEMU环境检测",
            "indicators": qemu_indicators,
            "is_likely_qemu": is_qemu,
            "confidence": "高" if sum(qemu_indicators.values()) >= 2 else "中" if any(qemu_indicators.values()) else "低",
            "explanation": "如果检测到QEMU环境，所有iKVM相关的失败都是预期的。"
        }

        self.results["evidence"]["qemu_detection"] = evidence
        return evidence

    def generate_report(self) -> str:
        """生成人类可读的报告"""
        report_lines = []
        report_lines.append("=" * 80)
        report_lines.append("QEMU AST2600 iKVM不可行性验证报告")
        report_lines.append("=" * 80)
        report_lines.append(f"\n时间戳: {self.results['timestamp']}")
        report_lines.append(f"平台: {self.results['platform']}")
        report_lines.append(f"\n检查项总数: {self.results['summary']['total_checks']}")
        report_lines.append(f"失败项: {self.results['summary']['failures']}")
        report_lines.append(f"预期失败项（QEMU限制）: {self.results['summary']['expected_failures']}")

        # 计算置信度
        if self.results['summary']['expected_failures'] >= 4:
            confidence = "非常高"
            conclusion = "✓ 证据充分：iKVM在QEMU AST2600上不可行"
        elif self.results['summary']['expected_failures'] >= 2:
            confidence = "高"
            conclusion = "✓ 证据较强：iKVM在QEMU AST2600上不可行"
        else:
            confidence = "低"
            conclusion = "⚠ 证据不足或环境可能不是QEMU"

        report_lines.append(f"\n置信度: {confidence}")
        report_lines.append(f"结论: {conclusion}")

        report_lines.append("\n" + "=" * 80)
        report_lines.append("详细证据")
        report_lines.append("=" * 80)

        for key, evidence in self.results["evidence"].items():
            report_lines.append(f"\n【{evidence.get('test_name', key)}】")
            if 'verdict' in evidence:
                report_lines.append(f"  判定: {evidence['verdict']}")
            if 'explanation' in evidence:
                report_lines.append(f"  说明: {evidence['explanation']}")

            # 显示关键细节
            if key == "video_device":
                report_lines.append(f"  设备存在: {evidence['exists']}")
            elif key == "hidg_devices":
                report_lines.append(f"  设备状态: {evidence['devices']}")
            elif key == "aspeed_video_driver":
                report_lines.append(f"  驱动已加载: {evidence['driver_loaded']}")
                report_lines.append(f"  超时错误: {evidence['timeout_errors_found']}")
                if evidence['sample_errors']:
                    report_lines.append(f"  示例错误: {evidence['sample_errors'][0][:120]}...")
            elif key == "ikvm_service":
                for svc, status in evidence['services'].items():
                    report_lines.append(f"  {svc}: {status['status']}")
            elif key == "vnc_port":
                report_lines.append(f"  端口5900监听: {evidence['listening']}")
            elif key == "qemu_detection":
                report_lines.append(f"  可能是QEMU: {evidence['is_likely_qemu']}")
                report_lines.append(f"  置信度: {evidence['confidence']}")

        report_lines.append("\n" + "=" * 80)
        report_lines.append("证明逻辑链")
        report_lines.append("=" * 80)
        report_lines.append("""
1. QEMU AST2600将Video Engine标记为TYPE_UNIMPLEMENTED_DEVICE
   ↓
2. aspeed-video驱动初始化时遇到超时（硬件不响应）
   ↓
3. /dev/video0设备文件未被创建
   ↓
4. obmc-ikvm无法打开/dev/video0，启动失败
   ↓
5. VNC服务器未在5900端口监听
   ↓
6. bmcweb无法连接到127.0.0.1:5900
   ↓
7. iKVM功能完全不可用

同理，USB Device Controller (UDC) 也是UnimplementedDeviceState：
- 无USB设备控制器硬件 → 无USB gadget框架 → 无/dev/hidg0 → obmc-ikvm失败
        """)

        report_lines.append("\n" + "=" * 80)
        report_lines.append("源代码证据位置")
        report_lines.append("=" * 80)
        report_lines.append("""
QEMU源码:
- hw/arm/aspeed_ast2600.c:130 (approx)
  object_initialize_child(obj, "video", &s->video, TYPE_UNIMPLEMENTED_DEVICE);

- include/hw/arm/aspeed_soc.h:70 (approx)
  UnimplementedDeviceState video;
  UnimplementedDeviceState udc;

Linux内核:
- drivers/media/platform/aspeed/aspeed-video.c
  aspeed_video_probe() 和超时处理

OpenBMC:
- github.com/openbmc/obmc-ikvm
  ikvm_video.cpp: Video::start() - open("/dev/video0")
  ikvm_input.cpp: Input::Input() - open("/dev/hidg0")

详细文档:
- 请查看 QEMU_AST2600_IKVM_IMPOSSIBILITY_PROOF.md
        """)

        report_lines.append("\n" + "=" * 80)
        report_lines.append("报告结束")
        report_lines.append("=" * 80)

        return "\n".join(report_lines)

    def run_all_checks(self):
        """运行所有检查"""
        print("开始收集证据...\n")

        checks = [
            ("检测QEMU环境", self.check_qemu_detection),
            ("检查Video设备", self.check_video_device),
            ("检查HID Gadget设备", self.check_hidg_devices),
            ("检查aspeed-video驱动", self.check_aspeed_video_driver),
            ("检查obmc-ikvm服务", self.check_ikvm_service),
            ("检查VNC端口", self.check_vnc_port),
            ("检查内核模块", self.check_kernel_modules),
            ("检查设备树", self.check_device_tree),
        ]

        for name, check_func in checks:
            print(f"正在执行: {name}...", end=" ")
            try:
                result = check_func()
                if result.get("verdict"):
                    status = "✓" if "证据确认" in result["verdict"] else "✗"
                    print(f"{status}")
                else:
                    print("完成")
            except Exception as e:
                print(f"错误: {e}")

        print("\n证据收集完成！\n")

    def save_json_report(self, filename: str = "ikvm_impossibility_evidence.json"):
        """保存JSON格式的报告"""
        with open(filename, "w", encoding="utf-8") as f:
            json.dump(self.results, f, indent=2, ensure_ascii=False)
        print(f"JSON报告已保存到: {filename}")

    def save_text_report(self, filename: str = "ikvm_impossibility_report.txt"):
        """保存文本格式的报告"""
        report = self.generate_report()
        with open(filename, "w", encoding="utf-8") as f:
            f.write(report)
        print(f"文本报告已保存到: {filename}")


def main():
    print("""
╔════════════════════════════════════════════════════════════════════════════╗
║           QEMU AST2600 iKVM不可行性自动验证工具                            ║
║                                                                            ║
║  此工具将收集证据证明iKVM无法在QEMU AST2600环境中运行                      ║
║  建议在QEMU模拟的OpenBMC环境中运行以获得最佳结果                           ║
╚════════════════════════════════════════════════════════════════════════════╝
    """)

    # 检查是否以root运行（某些检查可能需要）
    if os.geteuid() != 0:
        print("警告: 未以root权限运行，某些检查可能无法完成。")
        print("建议使用: sudo python3 verify_ikvm_impossibility.py\n")

    verifier = IKVMImpossibilityVerifier()

    try:
        # 运行所有检查
        verifier.run_all_checks()

        # 生成并显示报告
        report = verifier.generate_report()
        print(report)

        # 保存报告
        verifier.save_text_report()
        verifier.save_json_report()

        print("\n" + "=" * 80)
        print("验证完成！")
        print("=" * 80)

        # 返回适当的退出代码
        if verifier.results["summary"]["expected_failures"] >= 4:
            print("\n✓ 证据充分：已证明iKVM在QEMU AST2600上不可行")
            return 0
        else:
            print("\n⚠ 警告：证据不足或环境可能不是QEMU")
            return 1

    except KeyboardInterrupt:
        print("\n\n用户中断，退出...")
        return 130
    except Exception as e:
        print(f"\n错误: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
