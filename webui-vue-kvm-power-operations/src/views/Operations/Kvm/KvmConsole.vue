<template>
  <div class="kvm-console">
    <!-- KVM Console 主視窗 -->
    <b-container fluid>
      <b-row>
        <b-col>
          <page-title :description="$t('pageKvmConsole.description')">
            {{ $t('pageKvmConsole.title') }}
          </page-title>
        </b-col>
      </b-row>

      <!-- Server Power Operations 控制面板 -->
      <!-- 注意：無論 KVM 連接狀態如何，此面板都會顯示 -->
      <b-row class="mb-4">
        <b-col>
          <b-card class="power-operations-card">
            <template #header>
              <h5 class="mb-0">
                <icon-power class="mr-2" />
                Server Power Operations
              </h5>
            </template>

            <!-- 當前電源狀態顯示 -->
            <b-row class="mb-3">
              <b-col>
                <dl class="mb-0">
                  <dt>{{ $t('pageKvmConsole.powerStatus') }}</dt>
                  <dd data-test-id="powerServerOps-text-hostStatus">
                    <status-icon :status="serverPowerState" />
                    {{ serverPowerStateText }}
                  </dd>
                </dl>
              </b-col>
            </b-row>

            <!-- 電源操作按鈕組 -->
            <b-row>
              <b-col lg="6" class="mb-3">
                <!-- Power On 按鈕 -->
                <b-button
                  variant="success"
                  block
                  data-test-id="serverPowerOperations-button-powerOn"
                  :disabled="isOperationInProgress"
                  @click="confirmPowerOperation('On')"
                >
                  <icon-power class="mr-2" />
                  {{ $t('pageKvmConsole.powerOn') }}
                </b-button>
              </b-col>

              <b-col lg="6" class="mb-3">
                <!-- Graceful Restart 按鈕 -->
                <b-button
                  variant="warning"
                  block
                  data-test-id="serverPowerOperations-button-gracefulRestart"
                  :disabled="isOperationInProgress"
                  @click="confirmPowerOperation('GracefulRestart')"
                >
                  <icon-renew class="mr-2" />
                  {{ $t('pageKvmConsole.gracefulRestart') }}
                </b-button>
              </b-col>

              <b-col lg="6" class="mb-3">
                <!-- Force Restart 按鈕 -->
                <b-button
                  variant="warning"
                  block
                  data-test-id="serverPowerOperations-button-forceRestart"
                  :disabled="isOperationInProgress"
                  @click="confirmPowerOperation('ForceRestart')"
                >
                  <icon-renew class="mr-2" />
                  {{ $t('pageKvmConsole.forceRestart') }}
                </b-button>
              </b-col>

              <b-col lg="6" class="mb-3">
                <!-- Graceful Shutdown 按鈕 -->
                <b-button
                  variant="secondary"
                  block
                  data-test-id="serverPowerOperations-button-gracefulShutdown"
                  :disabled="isOperationInProgress"
                  @click="confirmPowerOperation('GracefulShutdown')"
                >
                  <icon-trashcan class="mr-2" />
                  {{ $t('pageKvmConsole.gracefulShutdown') }}
                </b-button>
              </b-col>

              <b-col lg="6" class="mb-3">
                <!-- Force Off 按鈕 -->
                <b-button
                  variant="danger"
                  block
                  data-test-id="serverPowerOperations-button-forceOff"
                  :disabled="isOperationInProgress"
                  @click="confirmPowerOperation('ForceOff')"
                >
                  <icon-trashcan class="mr-2" />
                  {{ $t('pageKvmConsole.forceOff') }}
                </b-button>
              </b-col>

              <b-col lg="6" class="mb-3">
                <!-- Power Cycle 按鈕 -->
                <b-button
                  variant="info"
                  block
                  data-test-id="serverPowerOperations-button-powerCycle"
                  :disabled="isOperationInProgress"
                  @click="confirmPowerOperation('PowerCycle')"
                >
                  <icon-renew class="mr-2" />
                  {{ $t('pageKvmConsole.powerCycle') }}
                </b-button>
              </b-col>
            </b-row>

            <!-- 操作中的提示 -->
            <b-row v-if="isOperationInProgress">
              <b-col>
                <b-alert show variant="info" class="mb-0">
                  <b-spinner small class="mr-2"></b-spinner>
                  {{ $t('pageKvmConsole.operationInProgress') }}
                </b-alert>
              </b-col>
            </b-row>
          </b-card>
        </b-col>
      </b-row>

      <!-- KVM 視窗區域 -->
      <b-row>
        <b-col>
          <b-card>
            <div id="kvm-screen" ref="kvmScreen" class="kvm-screen">
              <!-- RFB/noVNC 畫布會插入到這裡 -->
              <canvas ref="kvmCanvas"></canvas>
            </div>

            <!-- KVM 連接狀態 -->
            <div class="kvm-status mt-3">
              <b-badge :variant="kvmStatusVariant">
                {{ kvmStatusText }}
              </b-badge>
            </div>
          </b-card>
        </b-col>
      </b-row>
    </b-container>

    <!-- 確認對話框 -->
    <b-modal
      id="power-operation-confirm-modal"
      v-model="showConfirmModal"
      :title="$t('pageKvmConsole.confirmOperation')"
      @ok="executePowerOperation"
      @cancel="cancelPowerOperation"
    >
      <p>{{ confirmMessage }}</p>
    </b-modal>
  </div>
</template>

<script>
import PageTitle from '@/components/Global/PageTitle';
import StatusIcon from '@/components/Global/StatusIcon';
import IconPower from '@carbon/icons-vue/es/power/20';
import IconRenew from '@carbon/icons-vue/es/renew/20';
import IconTrashcan from '@carbon/icons-vue/es/trash-can/20';
import { mapState } from 'vuex';

export default {
  name: 'KvmConsole',
  components: {
    PageTitle,
    StatusIcon,
    IconPower,
    IconRenew,
    IconTrashcan,
  },
  data() {
    return {
      // KVM 連接狀態
      kvmConnected: false,
      kvmStatusText: 'Disconnected',

      // 伺服器電源狀態
      serverPowerState: 'off', // 'on', 'off', 'unknown'

      // 操作狀態
      isOperationInProgress: false,

      // 確認對話框
      showConfirmModal: false,
      pendingOperation: null,
      confirmMessage: '',

      // RFB 連接物件 (noVNC)
      rfb: null,
    };
  },
  computed: {
    ...mapState('authentication', ['consoleWindow']),

    /**
     * 計算電源狀態文字
     * @returns {string} 電源狀態的顯示文字
     */
    serverPowerStateText() {
      const stateMap = {
        on: this.$t('pageKvmConsole.powerStateOn'),
        off: this.$t('pageKvmConsole.powerStateOff'),
        unknown: this.$t('pageKvmConsole.powerStateUnknown'),
      };
      return stateMap[this.serverPowerState] || 'Unknown';
    },

    /**
     * 計算 KVM 狀態徽章顏色
     * @returns {string} Bootstrap variant
     */
    kvmStatusVariant() {
      return this.kvmConnected ? 'success' : 'secondary';
    },
  },
  mounted() {
    // 初始化 KVM 連接
    this.initKvmConnection();

    // 獲取當前電源狀態
    this.fetchPowerState();

    // 定期更新電源狀態（每 10 秒）
    this.powerStateInterval = setInterval(() => {
      this.fetchPowerState();
    }, 10000);
  },
  beforeDestroy() {
    // 清理定時器
    if (this.powerStateInterval) {
      clearInterval(this.powerStateInterval);
    }

    // 斷開 KVM 連接
    if (this.rfb) {
      this.rfb.disconnect();
    }
  },
  methods: {
    /**
     * 初始化 KVM 連接
     * 注意：在 QEMU 環境中可能無法連接，但不影響電源操作功能
     */
    initKvmConnection() {
      // KVM 連接邏輯（noVNC/RFB）
      // 由於 QEMU 限制，此處可能無法建立連接
      // 但電源操作按鈕仍然可用
      try {
        // 這裡應該有 noVNC 的初始化程式碼
        this.kvmStatusText = 'Connecting...';
        // ... RFB 連接邏輯 ...
      } catch (error) {
        console.error('KVM connection failed:', error);
        this.kvmStatusText = 'Connection Failed';
      }
    },

    /**
     * 獲取當前伺服器電源狀態
     * 透過 Redfish API 查詢
     */
    async fetchPowerState() {
      try {
        const response = await this.$store.dispatch('global/getSystemInfo');
        // 根據回應更新電源狀態
        if (response && response.PowerState) {
          this.serverPowerState = response.PowerState === 'On' ? 'on' : 'off';
        }
      } catch (error) {
        console.error('Failed to fetch power state:', error);
        this.serverPowerState = 'unknown';
      }
    },

    /**
     * 顯示電源操作確認對話框
     * @param {string} operation - 操作類型（On, GracefulRestart, ForceRestart, etc.）
     */
    confirmPowerOperation(operation) {
      this.pendingOperation = operation;

      // 根據操作類型設定確認訊息
      const messageMap = {
        On: this.$t('pageKvmConsole.confirmPowerOn'),
        GracefulRestart: this.$t('pageKvmConsole.confirmGracefulRestart'),
        ForceRestart: this.$t('pageKvmConsole.confirmForceRestart'),
        GracefulShutdown: this.$t('pageKvmConsole.confirmGracefulShutdown'),
        ForceOff: this.$t('pageKvmConsole.confirmForceOff'),
        PowerCycle: this.$t('pageKvmConsole.confirmPowerCycle'),
      };

      this.confirmMessage = messageMap[operation] || 'Confirm this operation?';
      this.showConfirmModal = true;
    },

    /**
     * 執行電源操作
     * 透過 Redfish API 發送指令
     */
    async executePowerOperation() {
      if (!this.pendingOperation) return;

      this.isOperationInProgress = true;

      try {
        // 呼叫 Redfish API: POST /redfish/v1/Systems/system/Actions/ComputerSystem.Reset/
        const response = await fetch(
          '/redfish/v1/Systems/system/Actions/ComputerSystem.Reset/',
          {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
              'X-Auth-Token': this.$store.getters['authentication/token'],
            },
            body: JSON.stringify({
              ResetType: this.pendingOperation,
            }),
          }
        );

        if (response.ok) {
          // 操作成功
          this.$bvToast.toast(
            this.$t('pageKvmConsole.operationSuccess'),
            {
              title: this.$t('pageKvmConsole.success'),
              variant: 'success',
              solid: true,
            }
          );

          // 立即更新電源狀態
          setTimeout(() => {
            this.fetchPowerState();
          }, 2000);
        } else {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
      } catch (error) {
        // 操作失敗
        console.error('Power operation failed:', error);
        this.$bvToast.toast(
          this.$t('pageKvmConsole.operationFailed') + ': ' + error.message,
          {
            title: this.$t('pageKvmConsole.error'),
            variant: 'danger',
            solid: true,
          }
        );
      } finally {
        this.isOperationInProgress = false;
        this.pendingOperation = null;
      }
    },

    /**
     * 取消電源操作
     */
    cancelPowerOperation() {
      this.pendingOperation = null;
      this.showConfirmModal = false;
    },
  },
};
</script>

<style lang="scss" scoped>
.kvm-console {
  padding: 1rem;
}

.power-operations-card {
  border-left: 4px solid $primary;

  .card-header {
    background-color: $light;
    border-bottom: 1px solid $border-color;

    h5 {
      display: flex;
      align-items: center;
    }
  }
}

.kvm-screen {
  position: relative;
  width: 100%;
  min-height: 600px;
  background-color: #000;
  border: 1px solid $border-color;
  display: flex;
  justify-content: center;
  align-items: center;

  canvas {
    max-width: 100%;
    max-height: 100%;
  }
}

.kvm-status {
  text-align: center;
}

// 按鈕懸停效果
.btn {
  transition: all 0.2s ease-in-out;

  &:hover:not(:disabled) {
    transform: translateY(-2px);
    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);
  }
}

// 操作中狀態
.b-spinner {
  vertical-align: middle;
}
</style>
