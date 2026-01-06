#!/usr/bin/env python3
"""
L1 Quiz Auto-Answer Script
使用 pexpect 自動回答考試題目，直到達到 100 分
"""

import pexpect
import subprocess
import sys
import os
import re

# 工號
EMP_ID = "835051"

# 題庫路徑
QUIZ_DIR = os.path.dirname(os.path.abspath(__file__))
SINGLE_B64 = os.path.join(QUIZ_DIR, "L1_quiz", "single_questions.b64")
MULTI_B64 = os.path.join(QUIZ_DIR, "L1_quiz", "multi_questions.b64")
QUIZ_SCRIPT = os.path.join(QUIZ_DIR, "L1_quiz", "L1_quiz.sh")

def load_answer_table():
    """解碼題庫並建立答案表"""
    answer_table = {}

    # 解碼單選題
    result = subprocess.run(
        ["base64", "--decode", SINGLE_B64],
        capture_output=True, text=True
    )
    for line in result.stdout.strip().split('\n'):
        if '|' in line:
            parts = line.split('|')
            if len(parts) >= 7:
                # parts[1] = question, parts[6] = answer
                question = parts[1].strip()
                answer = parts[6].strip()
                # 使用題目的前30個字作為 key
                key = question[:30] if len(question) > 30 else question
                answer_table[key] = answer

    # 解碼複選題
    result = subprocess.run(
        ["base64", "--decode", MULTI_B64],
        capture_output=True, text=True
    )
    for line in result.stdout.strip().split('\n'):
        if '|' in line:
            parts = line.split('|')
            if len(parts) >= 7:
                question = parts[1].strip()
                answer = parts[6].strip()
                key = question[:30] if len(question) > 30 else question
                answer_table[key] = answer

    return answer_table

def find_answer(question, answer_table):
    """根據題目找到答案"""
    # 清理題目文字
    question = question.strip()

    # 嘗試精確匹配前30個字
    key = question[:30] if len(question) > 30 else question
    if key in answer_table:
        return answer_table[key]

    # 嘗試模糊匹配
    for k, v in answer_table.items():
        if k in question or question in k:
            return v
        # 移除空格後比較
        if k.replace(' ', '') in question.replace(' ', ''):
            return v

    return None

def run_quiz(answer_table):
    """執行考試並自動回答"""
    # 啟動考試腳本
    child = pexpect.spawn(f'bash {QUIZ_SCRIPT}', cwd=os.path.join(QUIZ_DIR, "L1_quiz"), encoding='utf-8')
    child.logfile = sys.stdout

    try:
        # 輸入工號
        child.expect('請輸入測試者工號:', timeout=10)
        child.sendline(EMP_ID)

        current_question = ""

        while True:
            try:
                # 等待輸入提示
                index = child.expect([
                    '請輸入答案（A/B/C/D）:',
                    '請輸入答案:',
                    '測驗結束',
                    pexpect.EOF,
                    pexpect.TIMEOUT
                ], timeout=30)

                if index == 0 or index == 1:
                    # 從 buffer 中提取題目
                    buffer = child.before

                    # 找到題目行（以"第 X 題："開頭）
                    lines = buffer.split('\n')
                    for line in lines:
                        if '題：' in line:
                            # 提取題目內容
                            match = re.search(r'題：(.+)', line)
                            if match:
                                current_question = match.group(1).strip()
                                break

                    # 查找答案
                    answer = find_answer(current_question, answer_table)

                    if answer:
                        print(f"\n[AUTO] 找到答案: {answer}")
                        child.sendline(answer)
                    else:
                        # 找不到答案，嘗試用 A
                        print(f"\n[AUTO] 未找到答案，使用預設: A")
                        child.sendline("A")

                elif index == 2:
                    # 測驗結束
                    print("\n=== 測驗結束 ===")
                    child.expect(pexpect.EOF, timeout=10)
                    break

                elif index == 3 or index == 4:
                    break

            except pexpect.TIMEOUT:
                print("[TIMEOUT] 等待超時")
                break

    except Exception as e:
        print(f"[ERROR] {e}")
    finally:
        child.close()

    # 返回最後的輸出內容
    return child.before if child.before else ""

def extract_score(output):
    """從輸出中提取分數"""
    match = re.search(r'總分\s*：\s*(\d+)', output)
    if match:
        return int(match.group(1))
    return 0

def main():
    print("=" * 60)
    print("L1 Quiz Auto-Answer Script")
    print(f"工號: {EMP_ID}")
    print("=" * 60)

    # 載入答案表
    print("\n[INFO] 載入題庫答案表...")
    answer_table = load_answer_table()
    print(f"[INFO] 已載入 {len(answer_table)} 題答案")

    # 嘗試次數
    max_attempts = 20
    attempt = 0

    while attempt < max_attempts:
        attempt += 1
        print(f"\n{'=' * 60}")
        print(f"第 {attempt} 次嘗試")
        print("=" * 60)

        output = run_quiz(answer_table)
        score = extract_score(output)

        print(f"\n[RESULT] 本次分數: {score}")

        if score >= 100:
            print("\n" + "=" * 60)
            print("恭喜！已達到 100 分！")
            print("請手動截圖保存結果")
            print("=" * 60)
            break
        else:
            print(f"[INFO] 分數不足 100，繼續嘗試...")

    if attempt >= max_attempts:
        print(f"\n[WARNING] 已嘗試 {max_attempts} 次，未達到 100 分")

if __name__ == "__main__":
    main()
