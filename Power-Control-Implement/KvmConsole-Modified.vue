<template>
  <div :class="marginClass">
    <div ref="toolbar" class="kvm-toolbar">
      <b-row class="d-flex">
        <!-- 左側：狀態顯示 -->
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
          <!-- 🆕 新增：電源狀態顯示 -->
          <dl class="mb-2">
            <dt class="d-inline font-weight-bold mr-1">
              Power Status:
            </dt>
            <dd class="d-inline">
              <status-icon :status="hostStatusIcon" />
              <span class="d-none d-md-inline"> {{ hostStatus }}</span>
            </dd>
          </dl>
        </b-col>

        <!-- 右側：控制按鈕區 -->
        <b-col class="d-flex justify-content-end pr-1 align-items-center">
          <!-- 🆕 新增：Power Control 按鈕群組 -->
          <div class="power-control-buttons mr-3">
            <!-- 如果主機是關機狀態：顯示 Power On 按鈕 -->
            <b-button
              v-if="hostStatus === 'off'"
              variant="success"
              size="sm"
              type="button"
              :disabled="isOperationInProgress"
              @click="powerOn"
              class="mr-2"
            >
              <icon-power />
              Power On
            </b-button>

            <!-- 如果主機是開機狀態：顯示重啟和關機選項 -->
            <template v-else-if="hostStatus === 'on' || hostStatus === 'running'">
              <!-- Reboot 下拉選單 -->
              <b-dropdown
                variant="primary"
                size="sm"
                text="Reboot"
                :disabled="isOperationInProgress"
                class="mr-2"
              >
                <b-dropdown-item @click="confirmReboot('orderly')">
                  <icon-restart class="mr-2" />
                  Orderly Reboot
                </b-dropdown-item>
                <b-dropdown-item @click="confirmReboot('immediate')">
                  <icon-restart class="mr-2" />
                  Immediate Reboot
                </b-dropdown-item>
              </b-dropdown>

              <!-- Shutdown 下拉選單 -->
              <b-dropdown
                variant="danger"
                size="sm"
                text="Shutdown"
                :disabled="isOperationInProgress"
                class="mr-2"
              >
                <b-dropdown-item @click="confirmShutdown('orderly')">
                  <icon-power class="mr-2" />
                  Orderly Shutdown
                </b-dropdown-item>
                <b-dropdown-item @click="confirmShutdown('immediate')">
                  <icon-power class="mr-2" />
                  Immediate Shutdown
                </b-dropdown-item>
              </b-dropdown>
            </template>

            <!-- 操作進行中的提示 -->
            <b-spinner
              v-if="isOperationInProgress"
              small
              variant="primary"
              class="mr-2"
            ></b-spinner>
          </div>

          <!-- 原有的按鈕 -->
          <b-button
            variant="link"
            type="button"
            @click="sendCtrlAltDel"
            size="sm"
            class="mr-2"
          >
            <icon-arrow-down />
            {{ $t('pageKvm.buttonCtrlAltDelete') }}
          </b-button>

          <b-button
            v-if="!isFullWindow"
            variant="link"
            type="button"
            @click="openConsoleWindow()"
            size="sm"
          >
            <icon-launch />
            {{ $t('pageKvm.openNewTab') }}
          </b-button>
        </b-col>
      </b-row>
    </div>

    <!-- KVM 畫面顯示區域 -->
    <div id="terminal-kvm" ref="panel" :class="terminalClass"></div>
  </div>
</template>

<script>
import RFB from '@novnc/novnc/core/rfb';
import StatusIcon from '@/components/Global/StatusIcon';
import IconLaunch from '@carbon/icons-vue/es/launch/20';
import IconArrowDown from '@carbon/icons-vue/es/arrow--down/16';
import IconPower from '@carbon/icons-vue/es/power/16';  // 🆕 新增
import IconRestart from '@carbon/icons-vue/es/restart/16';  // 🆕 新增
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
    IconPower,    // 🆕 新增
    IconRestart,  // 🆕 新增
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
    // KVM 連線狀態圖示
    serverStatusIcon() {
      if (this.status === Connected) {
        return 'success';
      } else if (this.status === Disconnected) {
        return 'danger';
      }
      return 'secondary';
    },
    // KVM 連線狀態文字
    serverStatus() {
      if (this.status === Connected) {
        return i18n.global.t('pageKvm.connected');
      } else if (this.status === Disconnected) {
        return i18n.global.t('pageKvm.disconnected');
      }
      return i18n.global.t('pageKvm.connecting');
    },
    // 🆕 新增：主機電源狀態
    hostStatus() {
      const status = this.$store.getters['global/serverStatus'];
      return status || 'unknown';
    },
    // 🆕 新增：主機電源狀態圖示
    hostStatusIcon() {
      const status = this.hostStatus;
      if (status === 'on' || status === 'running') {
        return 'success';
      } else if (status === 'off') {
        return 'danger';
      }
      return 'secondary';
    },
    // 🆕 新增：是否正在執行電源操作
    isOperationInProgress() {
      return this.$store.getters['controls/isOperationInProgress'];
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
    // ========================================
    // 原有的 KVM 控制方法
    // ========================================
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
      if (this.$eventBus.$consoleWindow) {
        if (!this.$eventBus.$consoleWindow.closed) {
          this.$eventBus.$consoleWindow.focus();
          return;
        } else {
          this.openNewWindow();
        }
      } else {
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
    // 🆕 新增：Power Control 方法
    // ========================================

    /**
     * Power On - 開機
     */
    async powerOn() {
      console.log('🔵 Power On 被點擊');
      try {
        await this.$store.dispatch('controls/serverPowerOn');
        this.$bvToast.toast('開機命令已發送', {
          title: '✅ Power Control',
          variant: 'success',
          autoHideDelay: 3000,
          solid: true,
        });
      } catch (error) {
        console.error('Power On 失敗:', error);
        this.$bvToast.toast(`開機失敗: ${error.message}`, {
          title: '❌ Error',
          variant: 'danger',
          autoHideDelay: 5000,
          solid: true,
        });
      }
    },

    /**
     * 確認重啟操作
     * @param {string} type - 'orderly' 或 'immediate'
     */
    confirmReboot(type) {
      const message =
        type === 'orderly'
          ? 'Are you sure you want to reboot the server gracefully? The operating system will be notified.'
          : 'Are you sure you want to force reboot the server immediately? This may cause data loss.';

      const title =
        type === 'orderly' ? 'Confirm Orderly Reboot' : 'Confirm Immediate Reboot';

      this.$bvModal
        .msgBoxConfirm(message, {
          title: title,
          okTitle: 'Confirm',
          okVariant: type === 'orderly' ? 'primary' : 'warning',
          cancelTitle: 'Cancel',
          centered: true,
        })
        .then((confirmed) => {
          if (confirmed) {
            this.rebootServer(type);
          }
        });
    },

    /**
     * 執行重啟
     * @param {string} type - 'orderly' 或 'immediate'
     */
    async rebootServer(type) {
      console.log(`🔄 Reboot (${type}) 被點擊`);
      try {
        if (type === 'orderly') {
          await this.$store.dispatch('controls/serverSoftReboot');
          this.$bvToast.toast('優雅重啟命令已發送', {
            title: '✅ Power Control',
            variant: 'success',
            autoHideDelay: 3000,
            solid: true,
          });
        } else {
          await this.$store.dispatch('controls/serverHardReboot');
          this.$bvToast.toast('強制重啟命令已發送', {
            title: '✅ Power Control',
            variant: 'warning',
            autoHideDelay: 3000,
            solid: true,
          });
        }
      } catch (error) {
        console.error('Reboot 失敗:', error);
        this.$bvToast.toast(`重啟失敗: ${error.message}`, {
          title: '❌ Error',
          variant: 'danger',
          autoHideDelay: 5000,
          solid: true,
        });
      }
    },

    /**
     * 確認關機操作
     * @param {string} type - 'orderly' 或 'immediate'
     */
    confirmShutdown(type) {
      const message =
        type === 'orderly'
          ? 'Are you sure you want to shut down the server gracefully? The operating system will be notified.'
          : 'Are you sure you want to force shut down the server immediately? This may cause data loss.';

      const title =
        type === 'orderly'
          ? 'Confirm Orderly Shutdown'
          : 'Confirm Immediate Shutdown';

      this.$bvModal
        .msgBoxConfirm(message, {
          title: title,
          okTitle: 'Confirm',
          okVariant: 'danger',
          cancelTitle: 'Cancel',
          centered: true,
        })
        .then((confirmed) => {
          if (confirmed) {
            this.shutdownServer(type);
          }
        });
    },

    /**
     * 執行關機
     * @param {string} type - 'orderly' 或 'immediate'
     */
    async shutdownServer(type) {
      console.log(`🔴 Shutdown (${type}) 被點擊`);
      try {
        if (type === 'orderly') {
          await this.$store.dispatch('controls/serverSoftPowerOff');
          this.$bvToast.toast('優雅關機命令已發送', {
            title: '✅ Power Control',
            variant: 'success',
            autoHideDelay: 3000,
            solid: true,
          });
        } else {
          await this.$store.dispatch('controls/serverHardPowerOff');
          this.$bvToast.toast('強制關機命令已發送', {
            title: '⚠️ Power Control',
            variant: 'warning',
            autoHideDelay: 3000,
            solid: true,
          });
        }
      } catch (error) {
        console.error('Shutdown 失敗:', error);
        this.$bvToast.toast(`關機失敗: ${error.message}`, {
          title: '❌ Error',
          variant: 'danger',
          autoHideDelay: 5000,
          solid: true,
        });
      }
    },
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

/* 🆕 新增：Power Control 按鈕樣式 */
.power-control-buttons {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}
</style>
