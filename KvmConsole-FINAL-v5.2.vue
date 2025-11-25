<template>
  <div :class="marginClass">
    <!-- [v5] Power Control Dropdown Button - 白色立體按鈕 -->
    <b-row v-if="isFullWindow" class="mb-2">
      <b-col>
        <b-dropdown
          id="power-operations-dropdown"
          variant="light"
          size="sm"
          class="power-dropdown"
          :disabled="isOperationInProgress"
        >
          <template #button-content>
            <icon-power class="mr-1" />
            {{ $t('pageKvm.powerControl.title') }}
          </template>
          <b-dropdown-item @click="powerOn">
            <icon-power class="mr-2" />
            {{ $t('pageServerPowerOperations.powerOn') }}
          </b-dropdown-item>
          <b-dropdown-item @click="confirmOperation('gracefulRestart')">
            <icon-restart class="mr-2" />
            {{ $t('pageServerPowerOperations.gracefulRestart') }}
          </b-dropdown-item>
          <b-dropdown-item @click="confirmOperation('forceRestart')">
            <icon-restart class="mr-2" />
            {{ $t('pageServerPowerOperations.forceRestart') }}
          </b-dropdown-item>
          <b-dropdown-divider></b-dropdown-divider>
          <b-dropdown-item @click="confirmOperation('gracefulShutdown')">
            <icon-stop class="mr-2" />
            {{ $t('pageServerPowerOperations.gracefulShutdown') }}
          </b-dropdown-item>
          <b-dropdown-item @click="confirmOperation('forceOff')">
            <icon-stop class="mr-2" />
            {{ $t('pageServerPowerOperations.forceOff') }}
          </b-dropdown-item>
        </b-dropdown>

        <!-- Operation in Progress Indicator -->
        <b-spinner
          v-if="isOperationInProgress"
          small
          class="ml-2"
          variant="info"
        ></b-spinner>
      </b-col>
    </b-row>
    <!-- [END v5] -->

    <!-- Original KVM Toolbar and Terminal - 保留原版功能 -->
    <div ref="toolbar" class="kvm-toolbar">
      <b-row class="d-flex">
        <b-col class="d-flex flex-column justify-content-end" cols="6">
          <!-- [v5.2 MODIFIED] Server status 同一行顯示 - 使用 nowrap -->
          <div class="server-status-line mb-2">
            <span class="font-weight-bold">
              {{ $t('pageKvm.powerControl.status') }}:
            </span>
            <status-icon :status="serverStatusIcon" class="ml-1" />
            <span class="ml-1">{{ serverStatus }}</span>
          </div>
          <!-- [END v5.2 MODIFIED] -->
        </b-col>

        <b-col class="d-flex justify-content-end pr-1">
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
// [ADD] Import Power Control Icons
import IconPower from '@carbon/icons-vue/es/power/20';
import IconRestart from '@carbon/icons-vue/es/restart/20';
import IconStop from '@carbon/icons-vue/es/stop--filled/20';
// [END ADD]
import { throttle } from 'lodash';
import { useI18n } from 'vue-i18n';
import i18n from '@/i18n';
// [ADD] Import mapState for Vuex
import { mapState } from 'vuex';
// [END ADD]

const Connecting = 0;
const Connected = 1;
const Disconnected = 2;

export default {
  name: 'KvmConsole',
  components: {
    StatusIcon,
    IconLaunch,
    IconArrowDown,
    // [ADD] Register Power Control Icons
    IconPower,
    IconRestart,
    IconStop,
    // [END ADD]
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
      // [ADD] Power Control State
      pendingOperation: null,
      // [END ADD]
    };
  },
  computed: {
    // [ADD] Map Vuex state for operation progress
    ...mapState('controls', ['isOperationInProgress']),
    // [END ADD]
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
    // [ADD] Power Control Methods
    powerOn() {
      // Power On doesn't need confirmation
      this.$store.dispatch('controls/serverPowerOn');
    },
    confirmOperation(operation) {
      // Show confirmation modal for destructive operations
      this.pendingOperation = operation;

      let modalMessage = '';
      let modalTitle = '';

      switch (operation) {
        case 'gracefulRestart':
          modalMessage = i18n.global.t(
            'pageServerPowerOperations.modal.confirmRebootMessage',
          );
          modalTitle = i18n.global.t(
            'pageServerPowerOperations.modal.confirmRebootTitle',
          );
          break;
        case 'forceRestart':
          modalMessage = i18n.global.t(
            'pageServerPowerOperations.modal.confirmRebootMessage',
          );
          modalTitle = i18n.global.t(
            'pageServerPowerOperations.modal.confirmRebootTitle',
          );
          break;
        case 'gracefulShutdown':
          modalMessage = i18n.global.t(
            'pageServerPowerOperations.modal.confirmShutdownMessage',
          );
          modalTitle = i18n.global.t(
            'pageServerPowerOperations.modal.confirmShutdownTitle',
          );
          break;
        case 'forceOff':
          modalMessage = i18n.global.t(
            'pageServerPowerOperations.modal.confirmShutdownMessage',
          );
          modalTitle = i18n.global.t(
            'pageServerPowerOperations.modal.confirmShutdownTitle',
          );
          break;
      }

      const modalOptions = {
        title: modalTitle,
        okTitle: i18n.global.t('global.action.confirm'),
        cancelTitle: i18n.global.t('global.action.cancel'),
        autoFocusButton: 'ok',
      };

      this.$bvModal
        .msgBoxConfirm(modalMessage, modalOptions)
        .then((confirmed) => {
          if (confirmed) {
            this.executePowerOperation();
          }
        });
    },
    executePowerOperation() {
      const operation = this.pendingOperation;

      switch (operation) {
        case 'gracefulRestart':
          this.$store.dispatch('controls/serverSoftReboot');
          break;
        case 'forceRestart':
          this.$store.dispatch('controls/serverHardReboot');
          break;
        case 'gracefulShutdown':
          this.$store.dispatch('controls/serverSoftPowerOff');
          break;
        case 'forceOff':
          this.$store.dispatch('controls/serverHardPowerOff');
          break;
      }

      this.pendingOperation = null;
    },
    // [END ADD]
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

// [v5.2] Server status 強制同一行顯示
.server-status-line {
  display: flex;
  align-items: center;
  flex-wrap: nowrap;
  white-space: nowrap;
}
// [END v5.2]

// [v5.1] White Button with Shadow - 白色立體按鈕 (Vue 3 :deep() 語法)
.power-dropdown {
  :deep(.btn) {
    display: inline-flex;
    align-items: center;
    padding: 0.375rem 0.75rem;
    font-size: 0.875rem;
    background-color: #ffffff;
    border: 1px solid #d0d0d0;
    color: #333333;
    box-shadow:
      0 1px 3px rgba(0, 0, 0, 0.12),
      0 1px 2px rgba(0, 0, 0, 0.1);
    transition: all 0.15s ease-in-out;

    &:hover {
      background-color: #f5f5f5;
      border-color: #c0c0c0;
      box-shadow:
        0 2px 4px rgba(0, 0, 0, 0.15),
        0 1px 3px rgba(0, 0, 0, 0.1);
    }

    &:focus {
      box-shadow: 0 0 0 0.2rem rgba(108, 117, 125, 0.25);
    }

    &:active {
      background-color: #e9e9e9;
      box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.1);
    }
  }
}

.dropdown-item {
  display: flex;
  align-items: center;
  padding: 0.5rem 1.5rem;

  &:hover {
    background-color: $gray-200;
  }
}
// [END v5.1]
</style>
