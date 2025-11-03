<template>
  <div :class="marginClass">
    <div ref="toolbar" class="kvm-toolbar">
      <b-row class="d-flex">
        <b-col class="d-flex flex-column justify-content-end" cols="4">
          <dl class="mb-2" sm="2" md="2">
            <dt class="d-inline font-weight-bold mr-1">
              {{ $t('pageKvm.status') }}:
            </dt>
            <dd class="d-inline">
              <status-icon :status="serverStatusIcon" />
              <span class="d-none d-md-inline"> {{ serverStatus }}</span>
            </dd>
          </dl>
        </b-col>

        <b-col class="d-flex justify-content-end pr-1">
          <!-- ========================================== -->
          <!-- 🆕 新增: Power Control 按鈕 (方案 A)       -->
          <!-- ========================================== -->
          <b-button
            v-if="isConnected"
            variant="success"
            size="sm"
            type="button"
            @click="powerOn"
            class="mr-2"
          >
            <icon-power />
            Power On
          </b-button>

          <b-button
            v-if="isConnected"
            variant="danger"
            size="sm"
            type="button"
            @click="powerOff"
            class="mr-2"
          >
            <icon-power />
            Power Off
          </b-button>
          <!-- ========================================== -->
          <!-- 結束: Power Control 按鈕                   -->
          <!-- ========================================== -->

          <!-- 原有的按鈕 -->
          <b-button
            v-if="isConnected"
            variant="link"
            type="button"
            @click="sendCtrlAltDel"
          >
            <icon-arrow-down />
            {{ $t('pageKvm.buttonCtrlAltDelete') }}
          </b-button>
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
    <div id="terminal-kvm" ref="panel" :class="terminalClass"></div>
  </div>
</template>

<script>
import RFB from '@novnc/novnc/core/rfb';
import StatusIcon from '@/components/Global/StatusIcon';
import IconLaunch from '@carbon/icons-vue/es/launch/20';
import IconArrowDown from '@carbon/icons-vue/es/arrow--down/16';
import IconPower from '@carbon/icons-vue/es/power/16'; // 🆕 新增 Power 圖標
import { throttle } from 'lodash';
import { useI18n } from 'vue-i18n';
import i18n from '@/i18n';

const Connecting = 0;
const Connected = 1;
const Disconnected = 2;

export default {
  name: 'KvmConsole',
  components: {
    StatusIcon,
    IconLaunch,
    IconArrowDown,
    IconPower, // 🆕 註冊 Power 圖標
  },
  props: {
    isFullWindow: {
      type: Boolean,
      default: true,
    },
  },
  data() {
    return {
      $t: useI18n().t,
      rfb: null,
      isConnected: false,
      terminalClass: this.isFullWindow ? 'full-window' : '',
      marginClass: this.isFullWindow ? 'margin-left-full-window' : '',
      status: Connecting,
      convasRef: null,
      resizeKvmWindow: null,
    };
  },
  computed: {
    serverStatusIcon() {
      if (this.status === Connected) {
        return 'success';
      } else if (this.status === Disconnected) {
        return 'danger';
      }
      return 'secondary';
    },
    serverStatus() {
      if (this.status === Connected) {
        return i18n.global.t('pageKvm.connected');
      } else if (this.status === Disconnected) {
        return i18n.global.t('pageKvm.disconnected');
      }
      return i18n.global.t('pageKvm.connecting');
    },
  },
  created() {
    this.$store.dispatch('global/getSystemInfo');
  },
  mounted() {
    this.openTerminal();
  },
  beforeUnmount() {
    window.removeEventListener('resize', this.resizeKvmWindow);
    this.closeTerminal();
  },
  methods: {
    sendCtrlAltDel() {
      this.rfb.sendCtrlAltDel();
    },
    closeTerminal() {
      this.rfb.disconnect();
      this.rfb = null;
    },
    openTerminal() {
      const token = this.$store.getters['authentication/token'];
      this.rfb = new RFB(
        this.$refs.panel,
        `wss://${window.location.host}/kvm/0`,
        { wsProtocols: [token] },
      );

      this.rfb.scaleViewport = true;
      this.rfb.clipViewport = true;
      const that = this;

      this.resizeKvmWindow = throttle(() => {
        setTimeout(that.setWidthToolbar, 0);
      }, 1000);
      window.addEventListener('resize', this.resizeKvmWindow);

      this.rfb.addEventListener('connect', () => {
        that.isConnected = true;
        that.status = Connected;
        that.setWidthToolbar();
      });

      this.rfb.addEventListener('disconnect', () => {
        this.isConnected = false;
        that.status = Disconnected;
      });
    },
    setWidthToolbar() {
      if (
        this.$refs.panel.children &&
        this.$refs.panel.children.length > 0 &&
        this.$refs.panel.children[0].children.length > 0
      ) {
        this.$refs.toolbar.style.width =
          this.$refs.panel.children[0].children[0].clientWidth - 10 + 'px';
      }
    },
    openConsoleWindow() {
      // If consoleWindow is not null
      // Check the newly opened window is closed or not
      if (this.$eventBus.$consoleWindow) {
        // If window is not closed set focus to new window
        // If window is closed, do open new window
        if (!this.$eventBus.$consoleWindow.closed) {
          this.$eventBus.$consoleWindow.focus();
          return;
        } else {
          this.openNewWindow();
        }
      } else {
        // If consoleWindow is null, open new window
        this.openNewWindow();
      }
    },
    openNewWindow() {
      this.$eventBus.$consoleWindow = window.open(
        '#/console/kvm',
        'kvmConsoleWindow',
        'directories=no,titlebar=no,toolbar=no,location=no,status=no,menubar=no,scrollbars=no,resizable=yes,width=700,height=550',
      );
    },

    // ========================================
    // 🆕 新增方法: Power Control
    // ========================================
    async powerOn() {
      console.group('🔵 ========== Power On 按鈕被點擊 ==========');
      console.log('📍 位置: KvmConsole.vue - powerOn() 方法');
      console.log('⏰ 時間:', new Date().toISOString());
      console.log('');

      console.log('📤 第1步: 準備呼叫 Vuex Store Action');
      console.log('   Action 名稱: controls/serverPowerOn');
      console.log('');

      try {
        // 呼叫 Vuex Store
        console.log('📤 第2步: 發送 dispatch 到 Vuex Store');
        await this.$store.dispatch('controls/serverPowerOn');

        console.log('');
        console.log('✅ 成功: Vuex Action 執行完成');
        console.log('📝 說明: 此 Action 會執行以下步驟:');
        console.log('   1. 設定 isOperationInProgress = true');
        console.log('   2. 呼叫 serverPowerChange({ ResetType: "On" })');
        console.log('   3. 發送 HTTP POST 到:');
        console.log('      URL: /redfish/v1/Systems/system/Actions/ComputerSystem.Reset');
        console.log('      Body: {"ResetType":"On"}');
        console.log('   4. 等待伺服器狀態變更為 "on"');
        console.log('   5. 設定 isOperationInProgress = false');
        console.log('');

        console.log('🔍 提示: 請查看 Network 分頁以確認 HTTP 請求');
        console.log('🔍 提示: 請查看 SSH 終端機以確認 bmcweb 日誌');
        console.groupEnd();

        // 顯示成功提示
        this.$bvToast.toast('開機命令已發送到 BMC', {
          title: '✅ Power Control',
          variant: 'success',
          autoHideDelay: 5000,
          appendToast: true,
        });

      } catch (error) {
        console.log('');
        console.error('❌ 錯誤: Power On 失敗');
        console.error('   錯誤訊息:', error.message);
        console.error('   完整錯誤:', error);
        console.groupEnd();

        // 顯示錯誤提示
        this.$bvToast.toast(`開機命令發送失敗: ${error.message}`, {
          title: '❌ Power Control Error',
          variant: 'danger',
          autoHideDelay: 8000,
          appendToast: true,
        });
      }
    },

    async powerOff() {
      console.group('🔴 ========== Power Off 按鈕被點擊 ==========');
      console.log('📍 位置: KvmConsole.vue - powerOff() 方法');
      console.log('⏰ 時間:', new Date().toISOString());
      console.log('');

      console.log('📤 第1步: 準備呼叫 Vuex Store Action');
      console.log('   Action 名稱: controls/serverSoftPowerOff');
      console.log('   說明: 這會執行優雅關機 (Graceful Shutdown)');
      console.log('');

      try {
        // 呼叫 Vuex Store
        console.log('📤 第2步: 發送 dispatch 到 Vuex Store');
        await this.$store.dispatch('controls/serverSoftPowerOff');

        console.log('');
        console.log('✅ 成功: Vuex Action 執行完成');
        console.log('📝 說明: 此 Action 會執行以下步驟:');
        console.log('   1. 設定 isOperationInProgress = true');
        console.log('   2. 呼叫 serverPowerChange({ ResetType: "GracefulShutdown" })');
        console.log('   3. 發送 HTTP POST 到:');
        console.log('      URL: /redfish/v1/Systems/system/Actions/ComputerSystem.Reset');
        console.log('      Body: {"ResetType":"GracefulShutdown"}');
        console.log('   4. 等待伺服器狀態變更為 "off"');
        console.log('   5. 設定 isOperationInProgress = false');
        console.log('');

        console.log('🔍 提示: 請查看 Network 分頁以確認 HTTP 請求');
        console.log('🔍 提示: 請查看 SSH 終端機以確認 bmcweb 日誌');
        console.groupEnd();

        // 顯示成功提示
        this.$bvToast.toast('關機命令已發送到 BMC（優雅關機）', {
          title: '✅ Power Control',
          variant: 'success',
          autoHideDelay: 5000,
          appendToast: true,
        });

      } catch (error) {
        console.log('');
        console.error('❌ 錯誤: Power Off 失敗');
        console.error('   錯誤訊息:', error.message);
        console.error('   完整錯誤:', error);
        console.groupEnd();

        // 顯示錯誤提示
        this.$bvToast.toast(`關機命令發送失敗: ${error.message}`, {
          title: '❌ Power Control Error',
          variant: 'danger',
          autoHideDelay: 8000,
          appendToast: true,
        });
      }
    },
    // ========================================
    // 結束: Power Control 方法
    // ========================================
  },
};
</script>

<style scoped lang="scss">
.button-ctrl-alt-delete {
  float: right;
}

.kvm-status {
  padding-top: $spacer / 2;
  padding-left: $spacer / 4;
  display: inline-block;
}

.margin-left-full-window {
  margin-left: 5px;
}
</style>
