/**
 * vue.config.js - Vue CLI 配置檔案
 *
 * 此檔案包含 webpack-dev-server 的修復配置
 * 解決 webpack-dev-server v4+ 棄用 'https' 選項的警告
 *
 * 主要修改：
 * - 從 devServer.https: true 改為 devServer.server.type: 'https'
 * - 適配 webpack-dev-server 4.x 新的 API
 */

const CompressionPlugin = require('compression-webpack-plugin');

module.exports = {
  // 公共路徑配置
  publicPath: process.env.NODE_ENV === 'production' ? './' : '/',

  // CSS 配置
  css: {
    loaderOptions: {
      sass: {
        // 全局引入變數和 mixins
        additionalData: `
          @import "@/assets/styles/_obmc-custom.scss";
        `,
      },
    },
  },

  // webpack-dev-server 配置
  devServer: {
    // ✅ 修復：使用新的 server 配置格式（webpack-dev-server 4.x+）
    // ❌ 舊版寫法（已棄用）：https: true
    server: {
      type: 'https', // 啟用 HTTPS
    },

    // 代理配置 - 將 API 請求轉發到 OpenBMC 後端
    proxy: {
      '/login': {
        target: process.env.BASE_URL || 'https://localhost:8443',
        changeOrigin: true,
        secure: false, // 忽略 SSL 憑證驗證（開發環境）
      },
      '/api': {
        target: process.env.BASE_URL || 'https://localhost:8443',
        changeOrigin: true,
        secure: false,
      },
      '/redfish': {
        target: process.env.BASE_URL || 'https://localhost:8443',
        changeOrigin: true,
        secure: false,
      },
      '/download': {
        target: process.env.BASE_URL || 'https://localhost:8443',
        changeOrigin: true,
        secure: false,
      },
      '/upload': {
        target: process.env.BASE_URL || 'https://localhost:8443',
        changeOrigin: true,
        secure: false,
      },
      '/subscribe': {
        target: process.env.BASE_URL || 'https://localhost:8443',
        ws: true, // 啟用 WebSocket 代理
        changeOrigin: true,
        secure: false,
      },
    },

    // 開發伺服器端口
    port: 8080,

    // 自動開啟瀏覽器
    open: false,

    // 啟用熱模組替換
    hot: true,

    // 主機配置
    host: 'localhost',

    // 覆寫層級
    historyApiFallback: true,

    // 允許的主機（安全性配置）
    allowedHosts: 'all',
  },

  // 生產環境配置
  configureWebpack: (config) => {
    if (process.env.NODE_ENV === 'production') {
      // 生產環境壓縮插件
      config.plugins.push(
        new CompressionPlugin({
          filename: '[path][base].gz',
          algorithm: 'gzip',
          test: /\.(js|css|html|svg)$/,
          threshold: 10240,
          minRatio: 0.8,
        })
      );
    }
  },

  // 轉譯依賴
  transpileDependencies: ['bootstrap-vue'],

  // Source map 配置
  productionSourceMap: false,

  // PWA 配置（如果使用）
  pwa: {
    name: 'OpenBMC Web UI',
    themeColor: '#0d6efd',
    msTileColor: '#000000',
    appleMobileWebAppCapable: 'yes',
    appleMobileWebAppStatusBarStyle: 'black',
  },
};
