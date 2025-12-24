/*
 * ============================================================================
 * power_control.cpp - L6 PowerControl 相關函數註解版
 * ============================================================================
 * 
 * 此檔案擷取自 OpenBMC x86-power-control 專案
 * 來源: https://github.com/openbmc/x86-power-control/blob/master/src/power_control.cpp
 * 
 * 本檔案僅包含 L6 作業 Task 1, 2, 3 相關的函數，並附上詳細繁體中文註解
 * 
 * Task 1: Reset Button 在 POWER ON 狀態的決策流程
 * Task 2: postCompleteHandler 的行為
 * Task 3: PowerCycle 事件如何執行
 * 
 * ============================================================================
 */

// ============================================================================
// 電源狀態列舉 (PowerState Enum)
// ============================================================================
// 
// 這個列舉定義了系統所有可能的電源狀態
// 狀態機會根據不同事件在這些狀態之間轉換
//
enum class PowerState
{
    on,                          // 系統開機中，正常運行
    waitForPowerOK,              // 等待 Power OK 訊號（開機過程中）
    waitForSIOPowerGood,         // 等待 SIO Power Good 訊號
    off,                         // 系統關機
    transitionToOff,             // 正在轉換到關機狀態（強制關機）
    gracefulTransitionToOff,     // 正在優雅地轉換到關機狀態（等待 OS 關機）
    cycleOff,                    // Power Cycle 的關機階段，等待計時器後重新開機
    transitionToCycleOff,        // 正在轉換到 Power Cycle 關機狀態（強制）
    gracefulTransitionToCycleOff,// 正在優雅地轉換到 Power Cycle 關機狀態
    checkForWarmReset,           // 檢查是否為 Warm Reset（不斷電重啟）
};

// ============================================================================
// 事件列舉 (Event Enum)
// ============================================================================
//
// 這個列舉定義了所有可能觸發狀態轉換的事件
// 事件可能來自 GPIO 訊號變化、計時器到期、或 D-Bus 請求
//
enum class Event
{
    powerOKAssert,               // Power OK 訊號變為有效（電源開啟成功）
    powerOKDeAssert,             // Power OK 訊號變為無效（電源關閉）
    sioPowerGoodAssert,          // SIO Power Good 訊號有效
    sioPowerGoodDeAssert,        // SIO Power Good 訊號無效
    sioS5Assert,                 // S5 狀態訊號有效（系統要進入 S5 關機狀態）
    sioS5DeAssert,               // S5 狀態訊號無效
    pltRstAssert,                // Platform Reset 訊號有效
    pltRstDeAssert,              // Platform Reset 訊號無效
    postCompleteAssert,          // POST Complete 訊號有效（BIOS POST 完成）
    postCompleteDeAssert,        // POST Complete 訊號無效（BIOS 正在 POST 或系統關閉）
    powerButtonPressed,          // 電源按鈕被按下
    resetButtonPressed,          // 重置按鈕被按下
    powerCycleTimerExpired,      // Power Cycle 計時器到期
    powerOKWatchdogTimerExpired, // Power OK Watchdog 計時器到期（開機超時）
    sioPowerGoodWatchdogTimerExpired, // SIO Power Good Watchdog 計時器到期
    gracefulPowerOffTimerExpired,// 優雅關機計時器到期（OS 關機超時）
    powerOnRequest,              // 開機請求（來自 D-Bus）
    powerOffRequest,             // 強制關機請求（來自 D-Bus）
    powerCycleRequest,           // 強制 Power Cycle 請求（來自 D-Bus）
    resetRequest,                // Reset 請求（來自 D-Bus）
    gracefulPowerOffRequest,     // 優雅關機請求（來自 D-Bus）
    gracefulPowerCycleRequest,   // 優雅 Power Cycle 請求（來自 D-Bus）
    warmResetDetected,           // 偵測到 Warm Reset（計時器到期確認）
};

// ============================================================================
// 作業系統狀態列舉 (OperatingSystemStateStage Enum)
// ============================================================================
//
// 定義作業系統的狀態，用於追蹤 BIOS POST 是否完成
//
enum class OperatingSystemStateStage
{
    Inactive,  // 非活動狀態：BIOS POST 尚未完成，或系統關閉
    Standby,   // 待機狀態：BIOS POST 完成，準備載入 OS
};

// ============================================================================
// 計時器設定值 (Timer Configuration)
// ============================================================================
//
// 這些計時器值定義了各種電源操作的時間參數
// 可透過 JSON 設定檔覆寫預設值
//
boost::container::flat_map<std::string, int> TimerMap = {
    {"PowerPulseMs", 200},           // 短按電源鍵時間（開機/優雅關機）：200ms
    {"ForceOffPulseMs", 15000},      // 長按電源鍵時間（強制關機）：15000ms = 15秒
    {"ResetPulseMs", 500},           // Reset 脈衝時間：500ms
    {"PowerCycleMs", 5000},          // Power Cycle 關機後等待時間：5000ms = 5秒
    {"SioPowerGoodWatchdogMs", 1000},// SIO Power Good Watchdog 超時：1秒
    {"PowerOKWatchdogMs", 8000},     // Power OK Watchdog 超時：8000ms = 8秒
    {"GracefulPowerOffS", (5 * 60)}, // 優雅關機超時：5分鐘 = 300秒
    {"WarmResetCheckMs", 500},       // Warm Reset 檢查時間：500ms
    {"PowerOffSaveMs", 7000},        // 電源狀態儲存延遲：7秒
    {"SlotPowerCycleMs", 200},       // Slot Power Cycle 時間：200ms
    {"DbusGetPropertyRetry", 1000}}; // D-Bus 屬性讀取重試間隔：1秒


// ============================================================================
// ============================================================================
//
//                            TASK 2 相關函數
//                       postCompleteHandler 的行為
//
// ============================================================================
// ============================================================================

// ============================================================================
// setOperatingSystemState() - 設定作業系統狀態
// ============================================================================
//
// 功能：更新作業系統狀態並發布到 D-Bus
// 
// 參數：
//   - stage: 要設定的作業系統狀態（Inactive 或 Standby）
//
// 呼叫時機：
//   - postCompleteHandler() 收到 POST Complete 訊號變化時
//
// 因果關係：
//   POST Complete Assert  → 呼叫此函數設為 Standby → D-Bus 更新 → Redfish/IPMI 可查詢
//   POST Complete DeAssert → 呼叫此函數設為 Inactive → D-Bus 更新 → Redfish/IPMI 可查詢
//
static void setOperatingSystemState(const OperatingSystemStateStage stage)
{
    // 檢查 OS Interface 是否已初始化
    // 如果 osIface 為空，表示 D-Bus 介面尚未建立，直接返回
    if (!osIface)
    {
        return;
    }

    // 更新全域變數，記錄目前的作業系統狀態
    operatingSystemState = stage;

// 條件編譯：如果啟用了「忽略 POST 期間軟重啟」功能
#if IGNORE_SOFT_RESETS_DURING_POST
    // 當 POST 完成（進入 Standby 狀態）時，清除忽略軟重啟的旗標
    // 這樣在 POST 完成後的軟重啟就會被正常處理
    if (operatingSystemState == OperatingSystemStateStage::Standby)
    {
        ignoreNextSoftReset = false;
    }
#endif

    // 透過 D-Bus 介面更新 OperatingSystemState 屬性
    // getOperatingSystemStateStage() 會將 enum 轉換為 D-Bus 標準字串
    // 例如："xyz.openbmc_project.State.OperatingSystem.Status.OSStatus.Standby"
    osIface->set_property("OperatingSystemState",
                          std::string(getOperatingSystemStateStage(stage)));

    // 記錄狀態變更到系統日誌
    lg2::info("Moving os state to {STATE} stage", "STATE",
              getOperatingSystemStateStage(stage));
}

// ============================================================================
// postCompleteHandler() - POST Complete 訊號處理函數
// ============================================================================
//
// 功能：處理 BIOS POST Complete 訊號的狀態變化
//
// 參數：
//   - state: GPIO 或 D-Bus 回報的訊號狀態（true/false）
//
// 觸發來源：
//   1. GPIO Type: POST_COMPLETE GPIO 腳位狀態變化
//   2. D-Bus Type: xyz.openbmc_project.Host.Misc.Manager 的 PostComplete 屬性變化
//      （來自 BIOS 透過 eSPI Virtual Wire 發送的 VW_FM_BIOS_POST_CMPLT_N 訊號）
//
// 因果關係：
//   BIOS POST 完成 → POST Complete GPIO/VW Assert → postCompleteHandler(true)
//     → asserted = true → 發送 postCompleteAssert 事件 → OS State = Standby
//
//   系統重啟/關機 → POST Complete GPIO/VW DeAssert → postCompleteHandler(false)
//     → asserted = false → 發送 postCompleteDeAssert 事件 → OS State = Inactive
//     → 如果目前在 PowerState::on，會進入 checkForWarmReset 狀態
//
static void postCompleteHandler(bool state)
{
    // 根據訊號極性判斷 POST Complete 是否為「已完成」狀態
    // postCompleteConfig.polarity 定義了「完成」對應的電平
    // 例如：如果 polarity = true，則 state = true 表示 POST 完成
    bool asserted = state == postCompleteConfig.polarity;
    
    if (asserted)
    {
        // POST Complete 被 Assert（BIOS POST 完成）
        
        // 1. 發送內部事件給狀態機
        //    狀態機會根據目前狀態決定如何處理此事件
        sendPowerControlEvent(Event::postCompleteAssert);
        
        // 2. 設定作業系統狀態為 Standby
        //    表示 BIOS 已完成硬體初始化，系統準備載入 Bootloader 和 OS
        setOperatingSystemState(OperatingSystemStateStage::Standby);
    }
    else
    {
        // POST Complete 被 DeAssert（BIOS 正在 POST 或系統關閉）
        
        // 1. 發送內部事件給狀態機
        //    如果系統目前在 ON 狀態，這可能表示 Warm Reset 正在進行
        sendPowerControlEvent(Event::postCompleteDeAssert);
        
        // 2. 設定作業系統狀態為 Inactive
        //    表示 BIOS POST 尚未完成
        setOperatingSystemState(OperatingSystemStateStage::Inactive);
    }
}


// ============================================================================
// ============================================================================
//
//                            TASK 1 相關函數
//               Reset Button 在 POWER ON 狀態的決策流程
//
// ============================================================================
// ============================================================================

// ============================================================================
// resetButtonHandler() - Reset 按鈕處理函數
// ============================================================================
//
// 功能：處理 Reset 按鈕的 GPIO 事件
//
// 參數：
//   - state: GPIO 回報的訊號狀態（true = 高電平, false = 低電平）
//
// 觸發來源：
//   Reset Button GPIO 發生 FALLING_EDGE 或 RISING_EDGE 事件時被呼叫
//
// 因果關係：
//   使用者按下 Reset Button → GPIO FALLING_EDGE → resetButtonHandler(false)
//     → asserted = true（假設 ActiveLow）→ 檢查 Mask
//       → 未被 Mask → 發送 resetButtonPressed 事件 → 記錄重啟原因
//       → 被 Mask → 僅記錄 log，不執行動作
//
//   使用者放開 Reset Button → GPIO RISING_EDGE → resetButtonHandler(true)
//     → asserted = false → 僅更新 D-Bus 屬性，不執行其他動作
//
static void resetButtonHandler(bool state)
{
    // 根據訊號極性判斷按鈕是否被「按下」
    // resetButtonConfig.polarity 定義了「按下」對應的電平
    // 大多數按鈕是 ActiveLow（按下時為低電平），所以 polarity = false
    // 當 state = false 且 polarity = false 時，asserted = true（按鈕被按下）
    bool asserted = state == resetButtonConfig.polarity;
    
    // 更新 D-Bus 屬性，讓其他服務（如 Redfish、WebUI）可以知道按鈕狀態
    resetButtonIface->set_property("ButtonPressed", asserted);
    
    // 只有在按鈕被「按下」時才執行後續動作
    if (asserted)
    {
        // 記錄 Reset Button 被按下的事件到系統日誌和 Redfish
        resetButtonPressLog();
        
        // 檢查 Reset Button 是否被 Mask（遮罩/禁用）
        // resetButtonMask 是一個 gpiod::line 物件，如果有設定則表示被 Mask
        // Mask 功能可透過 D-Bus 設定，用於暫時禁用實體按鈕
        if (!resetButtonMask)
        {
            // 未被 Mask，執行正常的 Reset 流程
            
            // 1. 發送 resetButtonPressed 事件給狀態機
            //    狀態機會根據目前的電源狀態決定如何處理
            //    如果目前是 PowerState::on，會進入 checkForWarmReset 狀態
            sendPowerControlEvent(Event::resetButtonPressed);
            
            // 2. 記錄重啟原因為「Reset Button」
            //    這個資訊會被儲存，供 IPMI/Redfish 查詢上次重啟的原因
            addRestartCause(RestartCause::resetButton);
        }
        else
        {
            // 被 Mask，僅記錄 log，不執行實際動作
            // 這可用於維護時暫時禁用實體按鈕，避免誤觸
            lg2::info("reset button press masked");
        }
    }
    
// 條件編譯：按鈕直通模式（Button Passthrough）
// 如果啟用此功能，BMC 會將按鈕狀態直接傳遞給主機
#if USE_BUTTON_PASSTHROUGH
    gpiod::line gpioLine;
    // 根據按鈕狀態設定輸出 GPIO 的電平
    bool outputState =
        asserted ? resetOutConfig.polarity : (!resetOutConfig.polarity);
    if (!setGPIOOutput(resetOutConfig.lineName, outputState, gpioLine))
    {
        lg2::error("{GPIO_NAME} reset button passthrough failed", "GPIO_NAME",
                   resetOutConfig.lineName);
    }
#endif
}

// ============================================================================
// warmResetCheckTimerStart() - 啟動 Warm Reset 檢查計時器
// ============================================================================
//
// 功能：啟動一個計時器來確認系統是否正在進行 Warm Reset
//
// 呼叫時機：
//   當系統處於 ON 狀態，收到 resetButtonPressed 或 postCompleteDeAssert 事件時
//
// 因果關係：
//   resetButtonPressed 事件 → powerStateOn() → setPowerState(checkForWarmReset)
//     → warmResetCheckTimerStart() → 等待 500ms
//       → 沒有其他事件 → 發送 warmResetDetected → 確認是 Warm Reset，回到 ON
//       → 收到 sioS5Assert → 取消計時器 → 系統要關機
//       → 收到 powerOKDeAssert → 取消計時器 → 電源異常斷開
//
static void warmResetCheckTimerStart()
{
    // 記錄計時器啟動
    lg2::info("Warm reset check timer started");
    
    // 設定計時器在 WarmResetCheckMs（預設 500ms）後到期
    warmResetCheckTimer.expires_after(
        std::chrono::milliseconds(TimerMap["WarmResetCheckMs"]));
    
    // 設定計時器到期時的回呼函數（非同步等待）
    warmResetCheckTimer.async_wait([](const boost::system::error_code ec) {
        if (ec)
        {
            // 如果錯誤碼是 operation_aborted，表示計時器被取消
            // 這是正常情況，例如收到 sioS5Assert 或 powerOKDeAssert 時會取消
            if (ec != boost::asio::error::operation_aborted)
            {
                // 其他錯誤則記錄到日誌
                lg2::error("Warm reset check async_wait failed: {ERROR_MSG}",
                           "ERROR_MSG", ec.message());
            }
            lg2::info("Warm reset check timer canceled");
            return;
        }
        
        // 計時器正常到期，表示在 500ms 內沒有收到關機或電源異常事件
        // 確認這是一個 Warm Reset（不斷電重啟）
        lg2::info("Warm reset check timer completed");
        
        // 發送 warmResetDetected 事件給狀態機
        // powerStateCheckForWarmReset() 會處理此事件，將狀態設回 ON
        sendPowerControlEvent(Event::warmResetDetected);
    });
}


// ============================================================================
// ============================================================================
//
//                            TASK 3 相關函數
//                       PowerCycle 事件如何執行
//
// ============================================================================
// ============================================================================

// ============================================================================
// powerOn() - 發送開機訊號
// ============================================================================
//
// 功能：透過 GPIO 發送短按電源鍵訊號來開機
//
// 因果關係：
//   powerStateCycleOff() 收到 powerCycleTimerExpired → powerOn()
//     → assertGPIOForMs(200ms) → 電源鍵短按 → 主機開始開機
//
static void powerOn()
{
    // assertGPIOForMs: 將 GPIO 設為有效電平，維持指定時間後恢復
    // powerOutConfig: 電源控制 GPIO 的設定
    // PowerPulseMs: 短按時間，預設 200ms
    assertGPIOForMs(powerOutConfig, TimerMap["PowerPulseMs"]);
}

// ============================================================================
// gracefulPowerOff() - 發送優雅關機訊號
// ============================================================================
//
// 功能：透過 GPIO 發送短按電源鍵訊號，通知 OS 進行正常關機程序
//
// 與 forcePowerOff() 的差異：
//   - gracefulPowerOff: 短按 200ms，OS 會收到 ACPI 電源按鈕事件，執行正常關機
//   - forcePowerOff: 長按 15 秒，強制切斷電源，類似拔電源線
//
// 因果關係：
//   gracefulPowerCycleRequest → gracefulPowerOff() → 短按 200ms
//     → OS 收到 ACPI 事件 → OS 開始關機程序 → 關機完成 → powerOKDeAssert
//
static void gracefulPowerOff()
{
    // 短按電源鍵，時間與開機相同（PowerPulseMs = 200ms）
    // OS 會將此視為關機請求，開始執行關機程序
    assertGPIOForMs(powerOutConfig, TimerMap["PowerPulseMs"]);
}

// ============================================================================
// forcePowerOff() - 強制關機
// ============================================================================
//
// 功能：透過 GPIO 發送長按電源鍵訊號來強制關機
//
// 原理：根據 ACPI 規範，長按電源鍵超過 4 秒會觸發強制關機
//       此函數使用 15 秒確保一定能關機
//
// 因果關係：
//   powerCycleRequest → forcePowerOff() → 長按 15 秒
//     → ACPI 強制關機 → 電源關閉 → powerOKDeAssert
//
static void forcePowerOff()
{
    // assertGPIOForMs: 將 GPIO 設為有效電平，維持指定時間後恢復
    // ForceOffPulseMs: 長按時間，預設 15000ms = 15 秒
    if (assertGPIOForMs(powerOutConfig, TimerMap["ForceOffPulseMs"]) < 0)
    {
        // GPIO 操作失敗，直接返回
        return;
    }

    // 設定一個回呼，當 GPIO assert 計時器到期時檢查結果
    gpioAssertTimer.async_wait([](const boost::system::error_code ec) {
        if (ec)
        {
            // operation_aborted 表示計時器被取消（正常情況，電源已關閉）
            if (ec != boost::asio::error::operation_aborted)
            {
                lg2::error("Force power off async_wait failed: {ERROR_MSG}",
                           "ERROR_MSG", ec.message());
            }
            return;
        }

        // 如果計時器正常到期但電源還沒關閉，表示強制關機失敗
        // 這是異常情況，可能是硬體問題
        lg2::error("Power-button override failed. Not sure what to do now.");
    });
}

// ============================================================================
// gracefulPowerOffTimerStart() - 啟動優雅關機超時計時器
// ============================================================================
//
// 功能：啟動一個計時器，等待 OS 完成關機程序
//       如果超時（預設 5 分鐘），則發送超時事件
//
// 因果關係：
//   gracefulPowerCycleRequest → gracefulPowerOffTimerStart() → 等待最多 5 分鐘
//     → OS 正常關機 → powerOKDeAssert → 取消此計時器 → 繼續 PowerCycle
//     → OS 未關機超時 → gracefulPowerOffTimerExpired → 回到 ON 狀態（失敗）
//
static void gracefulPowerOffTimerStart()
{
    lg2::info("Graceful power-off timer started");
    
    // 設定計時器，預設等待 5 分鐘（GracefulPowerOffS = 300 秒）
    gracefulPowerOffTimer.expires_after(
        std::chrono::seconds(TimerMap["GracefulPowerOffS"]));
    
    gracefulPowerOffTimer.async_wait([](const boost::system::error_code ec) {
        if (ec)
        {
            // operation_aborted 表示計時器被取消（OS 已正常關機）
            if (ec != boost::asio::error::operation_aborted)
            {
                lg2::error("Graceful power-off async_wait failed: {ERROR_MSG}",
                           "ERROR_MSG", ec.message());
            }
            lg2::info("Graceful power-off timer canceled");
            return;
        }
        
        // 計時器到期，OS 在 5 分鐘內未能完成關機
        lg2::info("Graceful power-off timer completed");
        
        // 發送超時事件，狀態機會處理此情況（通常是放棄並回到 ON 狀態）
        sendPowerControlEvent(Event::gracefulPowerOffTimerExpired);
    });
}

// ============================================================================
// powerCycleTimerStart() - 啟動 Power Cycle 等待計時器
// ============================================================================
//
// 功能：在關機後等待一段時間再開機
//       這個等待時間讓電容放電、硬體狀態重置
//
// 因果關係：
//   powerOKDeAssert（電源已關）→ powerStateCycleOff/TransitionToCycleOff
//     → powerCycleTimerStart() → 等待 5 秒 → powerCycleTimerExpired
//     → powerOn() → 重新開機
//
static void powerCycleTimerStart()
{
    lg2::info("Power-cycle timer started");
    
    // 設定計時器，預設等待 5 秒（PowerCycleMs = 5000ms）
    powerCycleTimer.expires_after(
        std::chrono::milliseconds(TimerMap["PowerCycleMs"]));
    
    powerCycleTimer.async_wait([](const boost::system::error_code ec) {
        if (ec)
        {
            // 計時器被取消（例如使用者在等待期間手動開機）
            if (ec != boost::asio::error::operation_aborted)
            {
                lg2::error("Power-cycle async_wait failed: {ERROR_MSG}",
                           "ERROR_MSG", ec.message());
            }
            lg2::info("Power-cycle timer canceled");
            return;
        }
        
        // 等待時間結束，準備重新開機
        lg2::info("Power-cycle timer completed");
        
        // 發送計時器到期事件
        // powerStateCycleOff() 會處理此事件，呼叫 powerOn() 開機
        sendPowerControlEvent(Event::powerCycleTimerExpired);
    });
}

// ============================================================================
// powerOKWatchdogTimerStart() - 啟動 Power OK Watchdog 計時器
// ============================================================================
//
// 功能：監控開機過程，如果在指定時間內沒有收到 Power OK 訊號，則判定開機失敗
//
// 因果關係：
//   powerOn() → powerOKWatchdogTimerStart() → 等待最多 8 秒
//     → 收到 powerOKAssert → 取消計時器 → 開機成功
//     → 超時未收到 → powerOKWatchdogTimerExpired → 開機失敗，進入 OFF 狀態
//
static void powerOKWatchdogTimerStart()
{
    lg2::info("power OK watchdog timer started");
    
    // 設定 Watchdog 計時器，預設 8 秒（PowerOKWatchdogMs = 8000ms）
    powerOKWatchdogTimer.expires_after(
        std::chrono::milliseconds(TimerMap["PowerOKWatchdogMs"]));
    
    powerOKWatchdogTimer.async_wait([](const boost::system::error_code ec) {
        if (ec)
        {
            // operation_aborted 表示計時器被取消（收到 Power OK，開機成功）
            if (ec != boost::asio::error::operation_aborted)
            {
                lg2::error("power OK watchdog async_wait failed: {ERROR_MSG}",
                           "ERROR_MSG", ec.message());
            }
            lg2::info("power OK watchdog timer canceled");
            return;
        }
        
        // Watchdog 計時器到期，在 8 秒內沒有收到 Power OK
        // 判定開機失敗
        lg2::info("power OK watchdog timer expired");
        
        // 發送超時事件，狀態機會將狀態設為 OFF 並記錄錯誤
        sendPowerControlEvent(Event::powerOKWatchdogTimerExpired);
    });
}


// ============================================================================
// ============================================================================
//
//                          電源狀態處理函數
//                    (Power State Handler Functions)
//
// ============================================================================
// ============================================================================

// ============================================================================
// powerStateOn() - 處理「開機中」狀態的事件
// ============================================================================
//
// 功能：當系統處於 ON 狀態時，處理各種事件
//
// 這是 Task 1、Task 2、Task 3 的關鍵函數，因為大部分事件都是在系統 ON 時發生
//
static void powerStateOn(const Event event)
{
    // 記錄收到的事件（用於除錯）
    logEvent(__FUNCTION__, event);
    
    switch (event)
    {
        // ====================================================================
        // 電源異常事件
        // ====================================================================
        case Event::powerOKDeAssert:
            // Power OK 訊號消失，表示電源意外關閉
            // 這是異常情況（例如電源故障、使用者拔電源線）
            setPowerState(PowerState::off);
            // 發出警告音通知使用者
            beep(beepPowerFail);
            break;

        // ====================================================================
        // S5 狀態事件（系統要關機）
        // ====================================================================
        case Event::sioS5Assert:
            // 收到 S5 訊號，表示系統要進入 S5 狀態（Soft Off）
            // 這通常是 OS 發起的關機
            setPowerState(PowerState::transitionToOff);
#if IGNORE_SOFT_RESETS_DURING_POST
            // 如果 POST 尚未完成，設定忽略下一次軟重啟的旗標
            if (operatingSystemState != OperatingSystemStateStage::Standby)
            {
                ignoreNextSoftReset = true;
            }
#endif
            // 記錄重啟原因為軟重啟
            addRestartCause(RestartCause::softReset);
            break;

        // ====================================================================
        // TASK 2: POST Complete DeAssert 事件
        // ====================================================================
        // 當使用 PLT_RST 訊號時，使用 pltRstAssert 事件
        // 否則使用 postCompleteDeAssert 事件
#if USE_PLT_RST
        case Event::pltRstAssert:
#else
        case Event::postCompleteDeAssert:
#endif
            // POST Complete 變為無效，可能是：
            // 1. 系統正在進行 Warm Reset（BIOS 重新執行 POST）
            // 2. 系統正在關機
            // 進入 checkForWarmReset 狀態來確認是哪種情況
            setPowerState(PowerState::checkForWarmReset);
#if IGNORE_SOFT_RESETS_DURING_POST
            if (operatingSystemState != OperatingSystemStateStage::Standby)
            {
                ignoreNextSoftReset = true;
            }
#endif
            // 暫時記錄為軟重啟，之後可能會更新
            addRestartCause(RestartCause::softReset);
            // 啟動 500ms 計時器來確認是否為 Warm Reset
            warmResetCheckTimerStart();
            break;

        // ====================================================================
        // 電源按鈕被按下（優雅關機）
        // ====================================================================
        case Event::powerButtonPressed:
            // 使用者按下電源按鈕，開始優雅關機流程
            setPowerState(PowerState::gracefulTransitionToOff);
            // 啟動優雅關機計時器（等待 OS 關機）
            gracefulPowerOffTimerStart();
            break;

        // ====================================================================
        // 強制關機請求（來自 D-Bus）
        // ====================================================================
        case Event::powerOffRequest:
            // 收到強制關機請求
            setPowerState(PowerState::transitionToOff);
            // 執行強制關機（長按電源鍵 15 秒）
            forcePowerOff();
            break;

        // ====================================================================
        // 優雅關機請求（來自 D-Bus）
        // ====================================================================
        case Event::gracefulPowerOffRequest:
            // 收到優雅關機請求
            setPowerState(PowerState::gracefulTransitionToOff);
            // 啟動優雅關機計時器
            gracefulPowerOffTimerStart();
            // 發送短按電源鍵訊號，通知 OS 關機
            gracefulPowerOff();
            break;

        // ====================================================================
        // TASK 3: 強制 PowerCycle 請求（來自 D-Bus）
        // ====================================================================
        case Event::powerCycleRequest:
            // 收到強制 Power Cycle 請求
            // 進入 transitionToCycleOff 狀態（強制關機後重開）
            setPowerState(PowerState::transitionToCycleOff);
            // 執行強制關機
            forcePowerOff();
            break;

        // ====================================================================
        // TASK 3: 優雅 PowerCycle 請求（來自 D-Bus）
        // ====================================================================
        case Event::gracefulPowerCycleRequest:
            // 收到優雅 Power Cycle 請求
            // 進入 gracefulTransitionToCycleOff 狀態（等待 OS 關機後重開）
            setPowerState(PowerState::gracefulTransitionToCycleOff);
            // 啟動優雅關機計時器（等待 OS 關機，最多 5 分鐘）
            gracefulPowerOffTimerStart();
            // 發送短按電源鍵訊號，通知 OS 關機
            gracefulPowerOff();
            break;

        // ====================================================================
        // TASK 1: Reset 按鈕被按下
        // ====================================================================
        case Event::resetButtonPressed:
            // 使用者按下 Reset 按鈕
            // 進入 checkForWarmReset 狀態，確認系統是否正常重啟
            setPowerState(PowerState::checkForWarmReset);
            // 啟動 500ms 計時器
            warmResetCheckTimerStart();
            break;

        // ====================================================================
        // Reset 請求（來自 D-Bus，僅重啟不斷電）
        // ====================================================================
        case Event::resetRequest:
            // 收到 Reset 請求，直接發送 Reset 訊號
            // 不改變電源狀態，因為 Reset 不會斷電
            reset();
            break;

        // ====================================================================
        // 其他事件
        // ====================================================================
        default:
            // 不認識的事件或不需要處理的事件
            lg2::info("No action taken.");
            break;
    }
}

// ============================================================================
// powerStateWaitForPowerOK() - 處理「等待 Power OK」狀態的事件
// ============================================================================
//
// 功能：在開機過程中等待 Power OK 訊號
//
// 進入此狀態的情況：
//   1. PowerCycle 的重新開機階段
//   2. 一般開機流程
//
// 因果關係：
//   powerOn() → setPowerState(waitForPowerOK) → powerOKWatchdogTimerStart()
//     → 等待 powerOKAssert（成功）或 Watchdog 超時（失敗）
//
static void powerStateWaitForPowerOK(const Event event)
{
    logEvent(__FUNCTION__, event);
    
    switch (event)
    {
        case Event::powerOKAssert:
        {
            // 收到 Power OK 訊號，電源已穩定供應
            
            // 取消所有正在進行的 GPIO 和 Watchdog 計時器
            gpioAssertTimer.cancel();
            powerOKWatchdogTimer.cancel();
            
            // 根據是否啟用 SIO 決定下一個狀態
            if (sioEnabled == true)
            {
                // 如果啟用 SIO，還需要等待 SIO Power Good 訊號
                sioPowerGoodWatchdogTimerStart();
                setPowerState(PowerState::waitForSIOPowerGood);
            }
            else
            {
                // 未啟用 SIO，直接進入 ON 狀態
                // 開機完成！
                setPowerState(PowerState::on);
            }
            break;
        }
        
        case Event::powerOKWatchdogTimerExpired:
            // Watchdog 計時器到期，在 8 秒內沒有收到 Power OK
            // 開機失敗，進入 OFF 狀態
            setPowerState(PowerState::off);
            // 記錄開機失敗事件（會寫入 Redfish 日誌）
            powerOKFailedLog();
            break;
            
        case Event::sioPowerGoodAssert:
            // 直接收到 SIO Power Good，跳過等待 Power OK
            powerOKWatchdogTimer.cancel();
            setPowerState(PowerState::on);
            break;
            
        default:
            lg2::info("No action taken.");
            break;
    }
}

// ============================================================================
// powerStateCheckForWarmReset() - 處理「檢查 Warm Reset」狀態的事件
// ============================================================================
//
// 功能：確認系統是否正在進行 Warm Reset（不斷電重啟）
//
// 進入此狀態的情況：
//   1. TASK 1: 收到 resetButtonPressed 事件
//   2. TASK 2: 收到 postCompleteDeAssert 事件
//
// 因果關係：
//   進入此狀態 → 啟動 500ms 計時器 → 等待事件
//     → 500ms 到期（warmResetDetected）→ 確認是 Warm Reset → 回到 ON
//     → 收到 sioS5Assert → 系統要關機 → 進入 transitionToOff
//     → 收到 powerOKDeAssert → 電源異常斷開 → 進入 OFF 並發出警告音
//
static void powerStateCheckForWarmReset(const Event event)
{
    logEvent(__FUNCTION__, event);
    
    switch (event)
    {
        case Event::sioS5Assert:
            // 收到 S5 訊號，表示系統要關機（不是 Warm Reset）
            // 取消 Warm Reset 檢查計時器
            warmResetCheckTimer.cancel();
            // 進入關機流程
            setPowerState(PowerState::transitionToOff);
            break;
            
        case Event::warmResetDetected:
            // 計時器到期，在 500ms 內沒有收到關機或電源異常事件
            // 確認這是一個 Warm Reset，系統正常重啟中
            // 回到 ON 狀態
            setPowerState(PowerState::on);
            break;
            
        case Event::powerOKDeAssert:
            // Power OK 訊號消失，電源意外關閉
            // 這是異常情況
            warmResetCheckTimer.cancel();
            setPowerState(PowerState::off);
            // 發出警告音
            beep(beepPowerFail);
            break;
            
        default:
            lg2::info("No action taken.");
            break;
    }
}

// ============================================================================
// powerStateTransitionToCycleOff() - 處理「轉換到 Cycle Off」狀態的事件
// ============================================================================
//
// 功能：等待強制關機完成，然後進入 PowerCycle 的等待階段
//
// 進入此狀態的情況：
//   TASK 3: 收到 powerCycleRequest → forcePowerOff() → 等待電源關閉
//
// 因果關係：
//   powerCycleRequest → forcePowerOff() → 等待
//     → powerOKDeAssert（電源已關）→ 進入 cycleOff → 啟動等待計時器
//
static void powerStateTransitionToCycleOff(const Event event)
{
    logEvent(__FUNCTION__, event);
    
    switch (event)
    {
        case Event::powerOKDeAssert:
            // 電源已關閉，準備進入 PowerCycle 的等待階段
            
            // 取消任何正在進行的 GPIO 計時器
            gpioAssertTimer.cancel();
            
            // 進入 cycleOff 狀態
            setPowerState(PowerState::cycleOff);
            
            // 啟動 Power Cycle 計時器（等待 5 秒後重新開機）
            powerCycleTimerStart();
            break;
            
        default:
            lg2::info("No action taken.");
            break;
    }
}

// ============================================================================
// powerStateGracefulTransitionToCycleOff() - 處理「優雅轉換到 Cycle Off」狀態的事件
// ============================================================================
//
// 功能：等待 OS 優雅關機完成，然後進入 PowerCycle 的等待階段
//
// 進入此狀態的情況：
//   TASK 3: 收到 gracefulPowerCycleRequest → gracefulPowerOff() → 等待 OS 關機
//
// 因果關係：
//   gracefulPowerCycleRequest → gracefulPowerOff() → gracefulPowerOffTimerStart()
//     → OS 正常關機 → powerOKDeAssert → 進入 cycleOff → 啟動等待計時器
//     → 5 分鐘超時 → gracefulPowerOffTimerExpired → 回到 ON（失敗）
//
static void powerStateGracefulTransitionToCycleOff(const Event event)
{
    logEvent(__FUNCTION__, event);
    
    switch (event)
    {
        case Event::powerOKDeAssert:
            // OS 已正常關機，電源已關閉
            
            // 取消優雅關機計時器（因為已成功關機）
            gracefulPowerOffTimer.cancel();
            
            // 進入 cycleOff 狀態
            setPowerState(PowerState::cycleOff);
            
            // 啟動 Power Cycle 計時器（等待 5 秒後重新開機）
            powerCycleTimerStart();
            break;
            
        case Event::gracefulPowerOffTimerExpired:
            // 優雅關機超時（5 分鐘內 OS 沒有關機）
            // PowerCycle 失敗，回到 ON 狀態
            setPowerState(PowerState::on);
            break;
            
        case Event::powerOffRequest:
            // 收到強制關機請求，取消優雅關機，改為強制關機
            gracefulPowerOffTimer.cancel();
            setPowerState(PowerState::transitionToOff);
            forcePowerOff();
            break;
            
        case Event::powerCycleRequest:
            // 收到強制 PowerCycle 請求，取消優雅關機，改為強制 PowerCycle
            gracefulPowerOffTimer.cancel();
            setPowerState(PowerState::transitionToCycleOff);
            forcePowerOff();
            break;
            
        case Event::resetRequest:
            // 收到 Reset 請求，取消關機，執行 Reset
            gracefulPowerOffTimer.cancel();
            setPowerState(PowerState::on);
            reset();
            break;
            
        default:
            lg2::info("No action taken.");
            break;
    }
}

// ============================================================================
// powerStateCycleOff() - 處理「Cycle Off」狀態的事件
// ============================================================================
//
// 功能：在 PowerCycle 的等待階段處理事件
//       等待計時器到期後重新開機
//
// 進入此狀態的情況：
//   TASK 3: 強制或優雅關機完成後，進入此狀態等待 5 秒
//
// 因果關係：
//   電源關閉 → 進入 cycleOff → powerCycleTimerStart() → 等待 5 秒
//     → powerCycleTimerExpired → powerOn() → 等待 powerOKAssert
//
static void powerStateCycleOff(const Event event)
{
    logEvent(__FUNCTION__, event);
    
    switch (event)
    {
        case Event::powerOKAssert:
        {
            // 意外收到 Power OK（可能是使用者手動開機）
            // 取消 PowerCycle 計時器，進入正常開機流程
            powerCycleTimer.cancel();
            
            if (sioEnabled == true)
            {
                sioPowerGoodWatchdogTimerStart();
                setPowerState(PowerState::waitForSIOPowerGood);
            }
            else
            {
                setPowerState(PowerState::on);
            }
            break;
        }
        
        case Event::sioS5DeAssert:
            // 收到 S5 DeAssert，可能是手動開機
            powerCycleTimer.cancel();
            powerOKWatchdogTimerStart();
            setPowerState(PowerState::waitForPowerOK);
            break;
            
        case Event::powerButtonPressed:
            // 使用者按下電源按鈕，提前開機
            powerCycleTimer.cancel();
            powerOKWatchdogTimerStart();
            setPowerState(PowerState::waitForPowerOK);
            break;
            
        case Event::powerCycleTimerExpired:
            // PowerCycle 計時器到期（已等待 5 秒）
            // 開始重新開機流程
            
            // 啟動 Power OK Watchdog（監控開機是否成功）
            powerOKWatchdogTimerStart();
            
            // 進入等待 Power OK 狀態
            setPowerState(PowerState::waitForPowerOK);
            
            // 發送開機訊號
            powerOn();
            break;
            
        default:
            lg2::info("No action taken.");
            break;
    }
}

// ============================================================================
// sendPowerControlEvent() - 發送事件給狀態機
// ============================================================================
//
// 功能：根據目前的電源狀態，找到對應的處理函數並呼叫
//
// 這是狀態機的核心分派函數，所有事件都通過此函數路由到正確的處理函數
//
static void sendPowerControlEvent(const Event event)
{
    // 根據目前的 powerState 取得對應的處理函數
    // 例如：powerState == on → 返回 powerStateOn 函數
    std::function<void(const Event)> handler = getPowerStateHandler(powerState);
    
    if (handler == nullptr)
    {
        // 找不到處理函數，記錄錯誤
        lg2::error("Failed to find handler for power state: {STATE}", "STATE",
                   static_cast<int>(powerState));
        return;
    }
    
    // 呼叫對應的處理函數
    handler(event);
}

// ============================================================================
// getPowerStateHandler() - 取得狀態處理函數
// ============================================================================
//
// 功能：根據電源狀態返回對應的處理函數
//
static std::function<void(const Event)> getPowerStateHandler(PowerState state)
{
    switch (state)
    {
        case PowerState::on:
            return powerStateOn;
        case PowerState::waitForPowerOK:
            return powerStateWaitForPowerOK;
        case PowerState::waitForSIOPowerGood:
            return powerStateWaitForSIOPowerGood;
        case PowerState::off:
            return powerStateOff;
        case PowerState::transitionToOff:
            return powerStateTransitionToOff;
        case PowerState::gracefulTransitionToOff:
            return powerStateGracefulTransitionToOff;
        case PowerState::cycleOff:
            return powerStateCycleOff;
        case PowerState::transitionToCycleOff:
            return powerStateTransitionToCycleOff;
        case PowerState::gracefulTransitionToCycleOff:
            return powerStateGracefulTransitionToCycleOff;
        case PowerState::checkForWarmReset:
            return powerStateCheckForWarmReset;
        default:
            return nullptr;
    }
}
