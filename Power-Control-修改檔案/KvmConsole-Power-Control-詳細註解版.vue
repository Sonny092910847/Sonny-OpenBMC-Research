<template>
  <div :class="marginClass">
    <div ref="toolbar" class="kvm-toolbar">
      <b-row class="d-flex">
        <!-- ========================================== -->
        <!-- 左側：狀態顯示區域                          -->
        <!-- ========================================== -->
        <b-col class="d-flex flex-column justify-content-end" cols="4">
          <dl class="mb-2" sm="2" md="2">
            <dt class="d-inline font-weight-bold mr-1">
              {{ $t('pageKvm.status') }}:
            </dt>
            <dd class="d-inline">
              <!-- 顯示 KVM 連線狀態圖示 (success/danger/secondary) -->
              <status-icon :status="serverStatusIcon" />
              <!-- 顯示連線狀態文字 (connected/disconnected/connecting) -->
              <span class="d-none d-md-inline"> {{ serverStatus }}</span>
            </dd>
          </dl>
        </b-col>

        <!-- ========================================== -->
        <!-- 右側：按鈕控制區域                          -->
        <!-- ========================================== -->
        <b-col class="d-flex justify-content-end pr-1">

          <!-- ========================================== -->
          <!-- 🆕 新增功能：Power Control 按鈕 (方案 A)   -->
          <!-- ========================================== -->

          <!-- Power On 按鈕 -->
          <!--
            功能：發送開機命令到 BMC
            條件：只有在 KVM 連線成功時才顯示 (v-if="isConnected")
            樣式：綠色按鈕 (variant="success")
            尺寸：小尺寸 (size="sm")
            點擊事件：呼叫 powerOn() 方法
          -->
          <b-button
            v-if="isConnected"
            variant="success"
            size="sm"
            type="button"
            @click="powerOn"
            class="mr-2"
            title="發送開機命令 (ResetType: On)"
          >
            <icon-power />
            Power On
          </b-button>

          <!-- Power Off 按鈕 -->
          <!--
            功能：發送關機命令到 BMC
            條件：只有在 KVM 連線成功時才顯示
            樣式：紅色按鈕 (variant="danger")
            點擊事件：呼叫 powerOff() 方法
            說明：這會執行「優雅關機」(Graceful Shutdown)，
                  會先通知作業系統正常關機
          -->
          <b-button
            v-if="isConnected"
            variant="danger"
            size="sm"
            type="button"
            @click="powerOff"
            class="mr-2"
            title="發送優雅關機命令 (ResetType: GracefulShutdown)"
          >
            <icon-power />
            Power Off
          </b-button>

          <!-- ========================================== -->
          <!-- 結束：Power Control 按鈕                   -->
          <!-- ========================================== -->

          <!-- ========================================== -->
          <!-- 以下是原有的按鈕（未修改）                  -->
          <!-- ========================================== -->

          <!-- Ctrl+Alt+Del 按鈕 -->
          <!--
            功能：發送 Ctrl+Alt+Delete 按鍵組合到遠端主機
            用途：在 Windows 系統中可以打開工作管理員或登入畫面
          -->
          <b-button
            v-if="isConnected"
            variant="link"
            type="button"
            @click="sendCtrlAltDel"
          >
            <icon-arrow-down />
            {{ $t('pageKvm.buttonCtrlAltDelete') }}
          </b-button>

          <!-- 開啟新分頁按鈕 -->
          <!--
            功能：在新視窗開啟全螢幕的 KVM Console
            條件：只在非全螢幕模式時顯示 (v-if="!isFullWindow")
            說明：從 /operations/kvm 可以開啟 /console/kvm
          -->
          <b-button
            v-if="!isFullWindow"
            variant="link"
            type="button"
            @click="openConsoleWindow()"
          >
            <icon-launch />
            {{ $t('pageKvm.openNewTab') }}
          </b-button>
        </b-col>
      </b-row>
    </div>

    <!-- ========================================== -->
    <!-- KVM 畫面顯示區域                            -->
    <!-- ========================================== -->
    <!--
      這是 noVNC (VNC over WebSocket) 的顯示容器
      RFB (Remote Frame Buffer) 會將畫面渲染到這個 div 中
    -->
    <div id="terminal-kvm" ref="panel" :class="terminalClass"></div>
  </div>
</template>

<script>
// ========================================
// 套件引入
// ========================================

// noVNC 核心套件：提供 VNC over WebSocket 功能
import RFB from '@novnc/novnc/core/rfb';

// 全域元件
import StatusIcon from '@/components/Global/StatusIcon';

// Carbon Design System 圖示
import IconLaunch from '@carbon/icons-vue/es/launch/20';      // 啟動/開啟新視窗圖示
import IconArrowDown from '@carbon/icons-vue/es/arrow--down/16';  // 向下箭頭圖示
import IconPower from '@carbon/icons-vue/es/power/16';        // 🆕 電源圖示

// 工具函式
import { throttle } from 'lodash';  // 節流函式，限制函式執行頻率

// Vue i18n 國際化
import { useI18n } from 'vue-i18n';
import i18n from '@/i18n';

// ========================================
// 連線狀態常數定義
// ========================================
const Connecting = 0;     // 連線中
const Connected = 1;      // 已連線
const Disconnected = 2;   // 已斷線

// ========================================
// Vue 元件定義
// ========================================
export default {
  name: 'KvmConsole',

  // ========================================
  // 註冊子元件
  // ========================================
  components: {
    StatusIcon,
    IconLaunch,
    IconArrowDown,
    IconPower,  // 🆕 註冊電源圖示
  },

  // ========================================
  // Props（父元件傳入的屬性）
  // ========================================
  props: {
    // 是否為全螢幕模式
    // true: /console/kvm (全螢幕)
    // false: /operations/kvm (視窗化，有左側選單)
    isFullWindow: {
      type: Boolean,
      default: true,
    },
  },

  // ========================================
  // 元件內部狀態 (data)
  // ========================================
  data() {
    return {
      $t: useI18n().t,           // 國際化翻譯函式
      rfb: null,                 // noVNC RFB 物件實例
      isConnected: false,        // KVM 是否已連線

      // CSS 類別名稱（根據是否全螢幕決定）
      terminalClass: this.isFullWindow ? 'full-window' : '',
      marginClass: this.isFullWindow ? 'margin-left-full-window' : '',

      status: Connecting,        // 當前連線狀態
      convasRef: null,           // Canvas 參考（保留但未使用）
      resizeKvmWindow: null,     // 視窗調整大小的節流函式
    };
  },

  // ========================================
  // 計算屬性 (computed)
  // ========================================
  computed: {
    // 根據連線狀態返回對應的圖示狀態
    serverStatusIcon() {
      if (this.status === Connected) {
        return 'success';    // 綠色（已連線）
      } else if (this.status === Disconnected) {
        return 'danger';     // 紅色（已斷線）
      }
      return 'secondary';    // 灰色（連線中）
    },

    // 根據連線狀態返回對應的文字
    serverStatus() {
      if (this.status === Connected) {
        return i18n.global.t('pageKvm.connected');       // "已連線"
      } else if (this.status === Disconnected) {
        return i18n.global.t('pageKvm.disconnected');    // "已斷線"
      }
      return i18n.global.t('pageKvm.connecting');        // "連線中"
    },
  },

  // ========================================
  // 生命週期鉤子：created
  // ========================================
  // 元件實例創建後立即執行
  created() {
    // 從 Vuex Store 取得系統資訊
    // 這會發送 GET /redfish/v1/Systems/system 請求
    this.$store.dispatch('global/getSystemInfo');
  },

  // ========================================
  // 生命週期鉤子：mounted
  // ========================================
  // 元件掛載到 DOM 後執行
  mounted() {
    // 開啟 KVM 終端機連線
    this.openTerminal();
  },

  // ========================================
  // 生命週期鉤子：beforeUnmount (Vue 3)
  // ========================================
  // 元件卸載前執行清理工作
  beforeUnmount() {
    // 移除視窗調整大小的事件監聽器
    window.removeEventListener('resize', this.resizeKvmWindow);

    // 關閉 KVM 連線
    this.closeTerminal();
  },

  // ========================================
  // 方法 (methods)
  // ========================================
  methods: {
    // ========================================
    // 原有方法（未修改）
    // ========================================

    /**
     * 發送 Ctrl+Alt+Delete 組合鍵到遠端主機
     *
     * 說明：
     * - 使用 noVNC RFB 物件的內建方法
     * - 在 Windows 系統中會觸發安全選項畫面（工作管理員/登出等）
     */
    sendCtrlAltDel() {
      this.rfb.sendCtrlAltDel();
    },

    /**
     * 關閉 KVM 終端機連線
     *
     * 說明：
     * - 中斷 WebSocket 連線
     * - 釋放 RFB 物件
     */
    closeTerminal() {
      this.rfb.disconnect();
      this.rfb = null;
    },

    /**
     * 開啟 KVM 終端機連線
     *
     * 流程：
     * 1. 取得認證 Token
     * 2. 建立 WebSocket 連線到 /kvm/0
     * 3. 初始化 noVNC RFB 物件
     * 4. 設定連線事件監聽器
     * 5. 設定視窗調整大小監聽器
     */
    openTerminal() {
      // 從 Vuex Store 取得認證 Token
      const token = this.$store.getters['authentication/token'];

      // 建立 noVNC RFB 連線
      // wss://<BMC_IP>/kvm/0 - 安全的 WebSocket 連線
      this.rfb = new RFB(
        this.$refs.panel,  // 顯示容器
        `wss://${window.location.host}/kvm/0`,  // WebSocket URL
        { wsProtocols: [token] },  // 使用 Token 認證
      );

      // 啟用自動縮放視窗以符合容器大小
      this.rfb.scaleViewport = true;

      // 啟用裁剪視窗（只顯示可見區域）
      this.rfb.clipViewport = true;

      const that = this;  // 保存 this 參考

      // 建立節流的視窗調整大小處理函式
      // 1000ms 內最多執行一次，避免頻繁觸發
      this.resizeKvmWindow = throttle(() => {
        setTimeout(that.setWidthToolbar, 0);
      }, 1000);

      // 監聽視窗調整大小事件
      window.addEventListener('resize', this.resizeKvmWindow);

      // 監聽 RFB 連線成功事件
      this.rfb.addEventListener('connect', () => {
        that.isConnected = true;
        that.status = Connected;
        that.setWidthToolbar();  // 調整工具列寬度
      });

      // 監聽 RFB 斷線事件
      this.rfb.addEventListener('disconnect', () => {
        this.isConnected = false;
        that.status = Disconnected;
      });
    },

    /**
     * 設定工具列寬度以符合 KVM 畫面寬度
     *
     * 說明：
     * - 確保工具列與 KVM 畫面對齊
     * - 需要等待 DOM 元素渲染完成
     */
    setWidthToolbar() {
      if (
        this.$refs.panel.children &&
        this.$refs.panel.children.length > 0 &&
        this.$refs.panel.children[0].children.length > 0
      ) {
        // 設定工具列寬度 = KVM Canvas 寬度 - 10px
        this.$refs.toolbar.style.width =
          this.$refs.panel.children[0].children[0].clientWidth - 10 + 'px';
      }
    },

    /**
     * 開啟 Console 視窗（新分頁）
     *
     * 邏輯：
     * - 如果已經有開啟的視窗且未關閉，則將焦點切換到該視窗
     * - 否則開啟新視窗
     */
    openConsoleWindow() {
      // 檢查是否已有開啟的 Console 視窗
      if (this.$eventBus.$consoleWindow) {
        // 檢查該視窗是否已關閉
        if (!this.$eventBus.$consoleWindow.closed) {
          // 視窗仍開啟，將焦點切換到該視窗
          this.$eventBus.$consoleWindow.focus();
          return;
        } else {
          // 視窗已關閉，開啟新視窗
          this.openNewWindow();
        }
      } else {
        // 尚未開啟過視窗，開啟新視窗
        this.openNewWindow();
      }
    },

    /**
     * 開啟新的 KVM Console 視窗
     *
     * 說明：
     * - 開啟 #/console/kvm 路由（全螢幕模式）
     * - 設定視窗大小為 700x550
     * - 移除視窗的工具列、選單列等
     */
    openNewWindow() {
      this.$eventBus.$consoleWindow = window.open(
        '#/console/kvm',                    // 路由路徑
        'kvmConsoleWindow',                 // 視窗名稱
        'directories=no,titlebar=no,toolbar=no,location=no,status=no,menubar=no,scrollbars=no,resizable=yes,width=700,height=550',  // 視窗選項
      );
    },

    // ========================================
    // 🆕 新增方法：Power Control 功能
    // ========================================

    /**
     * Power On 按鈕點擊處理函式
     *
     * 功能：發送開機命令到 BMC
     *
     * 流程：
     * 1. 顯示詳細的 console.log 追蹤資訊
     * 2. 呼叫 Vuex Store 的 serverPowerOn action
     * 3. Vuex Store 會發送 HTTP POST 請求到 Redfish API
     * 4. 顯示成功或失敗的 Toast 提示
     *
     * API 詳細資訊：
     * - URL: POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset
     * - Body: { "ResetType": "On" }
     * - 對應的 D-Bus 命令: xyz.openbmc_project.State.Host.Transition.On
     */
    async powerOn() {
      // ========================================
      // 步驟 1: 顯示詳細的追蹤日誌
      // ========================================
      console.group('🔵 ========== Power On 按鈕被點擊 ==========');
      console.log('📍 位置: KvmConsole.vue - powerOn() 方法');
      console.log('⏰ 時間:', new Date().toISOString());
      console.log('');

      console.log('📤 第1步: 準備呼叫 Vuex Store Action');
      console.log('   Action 名稱: controls/serverPowerOn');
      console.log('   Store 路徑: @/store/modules/Operations/ControlStore.js');
      console.log('');

      try {
        // ========================================
        // 步驟 2: 呼叫 Vuex Store Action
        // ========================================
        console.log('📤 第2步: 發送 dispatch 到 Vuex Store');
        console.log('   等待 Action 執行完成...');
        console.log('');

        // 呼叫 Vuex Store 的 serverPowerOn action
        // 這是一個 async action，會等待完成
        await this.$store.dispatch('controls/serverPowerOn');

        // ========================================
        // 步驟 3: 顯示執行流程說明
        // ========================================
        console.log('✅ 成功: Vuex Action 執行完成');
        console.log('');
        console.log('📝 此 Action 執行的步驟:');
        console.log('   ┌─ 步驟 1: 設定 isOperationInProgress = true');
        console.log('   │');
        console.log('   ├─ 步驟 2: 呼叫 serverPowerChange({ ResetType: "On" })');
        console.log('   │');
        console.log('   ├─ 步驟 3: 發送 HTTP POST 請求');
        console.log('   │   URL: /redfish/v1/Systems/system/Actions/ComputerSystem.Reset');
        console.log('   │   Method: POST');
        console.log('   │   Headers:');
        console.log('   │     Content-Type: application/json');
        console.log('   │     X-Auth-Token: <your_token>');
        console.log('   │   Body:');
        console.log('   │     {"ResetType":"On"}');
        console.log('   │');
        console.log('   ├─ 步驟 4: bmcweb 接收請求並處理');
        console.log('   │   檔案: bmcweb/redfish-core/lib/systems.hpp');
        console.log('   │   函式: handleComputerSystemResetActionPost()');
        console.log('   │');
        console.log('   ├─ 步驟 5: bmcweb 發送 D-Bus 訊息');
        console.log('   │   Service: xyz.openbmc_project.State.Host');
        console.log('   │   Object: /xyz/openbmc_project/state/host0');
        console.log('   │   Property: RequestedHostTransition');
        console.log('   │   Value: xyz.openbmc_project.State.Host.Transition.On');
        console.log('   │');
        console.log('   ├─ 步驟 6: phosphor-state-manager 處理 D-Bus 訊息');
        console.log('   │   檔案: phosphor-state-manager/host_state_manager.cpp');
        console.log('   │   函式: Host::requestedHostTransition()');
        console.log('   │');
        console.log('   ├─ 步驟 7: 等待主機狀態變更為 "on"');
        console.log('   │   輪詢 CurrentHostState 屬性');
        console.log('   │   超時時間: 5 分鐘');
        console.log('   │');
        console.log('   └─ 步驟 8: 設定 isOperationInProgress = false');
        console.log('');
        console.log('🔍 如何查看完整的通訊流程:');
        console.log('   1. 瀏覽器 Network 分頁: 查看 HTTP POST 請求');
        console.log('   2. SSH 終端 (bmcweb): journalctl -u bmcweb -f');
        console.log('   3. SSH 終端 (D-Bus): dbus-monitor --system');
        console.log('   4. SSH 終端 (State Manager): journalctl -u xyz.openbmc_project.State.Host -f');
        console.groupEnd();

        // ========================================
        // 步驟 4: 顯示成功提示
        // ========================================
        this.$bvToast.toast('開機命令已成功發送到 BMC，請等待主機啟動', {
          title: '✅ Power Control - 開機',
          variant: 'success',      // 綠色提示
          autoHideDelay: 5000,     // 5 秒後自動消失
          appendToast: true,       // 追加到現有提示下方
          solid: true,             // 實心背景
        });

      } catch (error) {
        // ========================================
        // 錯誤處理
        // ========================================
        console.log('');
        console.error('❌ 錯誤: Power On 失敗');
        console.error('   錯誤類型:', error.name);
        console.error('   錯誤訊息:', error.message);
        console.error('   完整錯誤物件:', error);
        console.error('');
        console.error('💡 可能的原因:');
        console.error('   1. BMC 未回應（網路問題）');
        console.error('   2. 認證 Token 已過期');
        console.error('   3. 主機已經在開機狀態');
        console.error('   4. BMC 韌體異常');
        console.groupEnd();

        // 顯示錯誤提示
        this.$bvToast.toast(
          `開機命令發送失敗: ${error.message}`,
          {
            title: '❌ Power Control 錯誤',
            variant: 'danger',       // 紅色提示
            autoHideDelay: 8000,     // 8 秒後自動消失
            appendToast: true,
            solid: true,
          }
        );
      }
    },

    /**
     * Power Off 按鈕點擊處理函式
     *
     * 功能：發送優雅關機命令到 BMC
     *
     * 流程：
     * 1. 顯示詳細的 console.log 追蹤資訊
     * 2. 呼叫 Vuex Store 的 serverSoftPowerOff action
     * 3. Vuex Store 會發送 HTTP POST 請求到 Redfish API
     * 4. 顯示成功或失敗的 Toast 提示
     *
     * API 詳細資訊：
     * - URL: POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset
     * - Body: { "ResetType": "GracefulShutdown" }
     * - 對應的 D-Bus 命令: xyz.openbmc_project.State.Host.Transition.Off
     *
     * 說明：
     * - 這是「優雅關機」，會先通知作業系統正常關機
     * - 作業系統會保存資料、關閉程式後才關機
     * - 如果需要「強制關機」，應使用 serverHardPowerOff
     */
    async powerOff() {
      // ========================================
      // 步驟 1: 顯示詳細的追蹤日誌
      // ========================================
      console.group('🔴 ========== Power Off 按鈕被點擊 ==========');
      console.log('📍 位置: KvmConsole.vue - powerOff() 方法');
      console.log('⏰ 時間:', new Date().toISOString());
      console.log('');

      console.log('📤 第1步: 準備呼叫 Vuex Store Action');
      console.log('   Action 名稱: controls/serverSoftPowerOff');
      console.log('   說明: 這會執行「優雅關機」(Graceful Shutdown)');
      console.log('   ├─ 優雅關機: 會先通知作業系統正常關機');
      console.log('   └─ 強制關機: serverHardPowerOff (立即斷電)');
      console.log('');

      try {
        // ========================================
        // 步驟 2: 呼叫 Vuex Store Action
        // ========================================
        console.log('📤 第2步: 發送 dispatch 到 Vuex Store');
        console.log('   等待 Action 執行完成...');
        console.log('');

        // 呼叫 Vuex Store 的 serverSoftPowerOff action
        await this.$store.dispatch('controls/serverSoftPowerOff');

        // ========================================
        // 步驟 3: 顯示執行流程說明
        // ========================================
        console.log('✅ 成功: Vuex Action 執行完成');
        console.log('');
        console.log('📝 此 Action 執行的步驟:');
        console.log('   ┌─ 步驟 1: 設定 isOperationInProgress = true');
        console.log('   │');
        console.log('   ├─ 步驟 2: 呼叫 serverPowerChange({ ResetType: "GracefulShutdown" })');
        console.log('   │');
        console.log('   ├─ 步驟 3: 發送 HTTP POST 請求');
        console.log('   │   URL: /redfish/v1/Systems/system/Actions/ComputerSystem.Reset');
        console.log('   │   Method: POST');
        console.log('   │   Body:');
        console.log('   │     {"ResetType":"GracefulShutdown"}');
        console.log('   │');
        console.log('   ├─ 步驟 4: bmcweb 發送 D-Bus 訊息');
        console.log('   │   Service: xyz.openbmc_project.State.Host');
        console.log('   │   Property: RequestedHostTransition');
        console.log('   │   Value: xyz.openbmc_project.State.Host.Transition.Off');
        console.log('   │');
        console.log('   ├─ 步驟 5: phosphor-state-manager 通知作業系統關機');
        console.log('   │   作業系統會收到關機信號 (ACPI Signal)');
        console.log('   │   作業系統執行關機流程:');
        console.log('   │     1. 保存所有開啟的檔案');
        console.log('   │     2. 關閉所有執行中的程式');
        console.log('   │     3. 卸載檔案系統');
        console.log('   │     4. 通知 BMC 可以關閉電源');
        console.log('   │');
        console.log('   ├─ 步驟 6: 等待主機狀態變更為 "off"');
        console.log('   │   輪詢 CurrentHostState 屬性');
        console.log('   │   超時時間: 5 分鐘');
        console.log('   │');
        console.log('   └─ 步驟 7: 設定 isOperationInProgress = false');
        console.log('');
        console.log('⚠️  注意事項:');
        console.log('   - 優雅關機可能需要 30 秒到 2 分鐘');
        console.log('   - 如果作業系統無回應，可能需要使用「強制關機」');
        console.log('   - 在 QEMU 環境中，關機速度會比實體機器快');
        console.groupEnd();

        // ========================================
        // 步驟 4: 顯示成功提示
        // ========================================
        this.$bvToast.toast(
          '關機命令已成功發送到 BMC，作業系統將進行優雅關機程序',
          {
            title: '✅ Power Control - 優雅關機',
            variant: 'success',
            autoHideDelay: 5000,
            appendToast: true,
            solid: true,
          }
        );

      } catch (error) {
        // ========================================
        // 錯誤處理
        // ========================================
        console.log('');
        console.error('❌ 錯誤: Power Off 失敗');
        console.error('   錯誤類型:', error.name);
        console.error('   錯誤訊息:', error.message);
        console.error('   完整錯誤物件:', error);
        console.error('');
        console.error('💡 可能的原因:');
        console.error('   1. BMC 未回應（網路問題）');
        console.error('   2. 認證 Token 已過期');
        console.error('   3. 主機已經在關機狀態');
        console.error('   4. BMC 韌體異常');
        console.groupEnd();

        // 顯示錯誤提示
        this.$bvToast.toast(
          `關機命令發送失敗: ${error.message}`,
          {
            title: '❌ Power Control 錯誤',
            variant: 'danger',
            autoHideDelay: 8000,
            appendToast: true,
            solid: true,
          }
        );
      }
    },

    // ========================================
    // 結束：Power Control 方法
    // ========================================
  },
};
</script>

<style scoped lang="scss">
/* ========================================
   樣式定義
   ======================================== */

/* Ctrl+Alt+Delete 按鈕樣式 */
.button-ctrl-alt-delete {
  float: right;
}

/* KVM 狀態顯示樣式 */
.kvm-status {
  padding-top: $spacer / 2;
  padding-left: $spacer / 4;
  display: inline-block;
}

/* 全螢幕模式的左邊距 */
.margin-left-full-window {
  margin-left: 5px;
}
</style>
