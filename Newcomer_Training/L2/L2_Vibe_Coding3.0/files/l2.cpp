// SPDX-License-Identifier: Apache-2.0                 //授權
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <phosphor-logging/lg2.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <fstream>
#include <memory>
#include <string>

PHOSPHOR_LOG2_USING; // claude code沒使用到

namespace //預防名稱跨檔案重複定義
{
// D-Bus 名稱
constexpr auto serviceName = "xyz.openbmc_project.Training.L2";
constexpr auto objectPath = "/xyz/openbmc_project/training/l2";
constexpr auto interfaceName = "xyz.openbmc_project.Training.L2.Monitor";

// 監控設定 (檔案, threshold, 每隔兩秒檢查一次)
constexpr auto monitorFile = "/tmp/l2_monitor_value";
constexpr int64_t defaultThreshold = 50;
constexpr auto pollInterval = std::chrono::seconds(2);

class L2Monitor
{
  public:
    L2Monitor(boost::asio::io_context& io,
              std::shared_ptr<sdbusplus::asio::connection> conn) :  //建構函式
        io(io),                                                   
        conn(conn), server(conn), timer(io), currentValue(0),     
        threshold(defaultThreshold), alertActive(false)
    {
        setupDbusInterface();
        startMonitoring();
    }

  private:
    void setupDbusInterface()  //設定D-Bus
    {
        iface = server.add_interface(objectPath, interfaceName);

        iface->register_property_r<int64_t>(
            "Value", sdbusplus::vtable::property_::emits_change,
            [this](const auto&) { return currentValue; });

        iface->register_property_rw<int64_t>(
            "Threshold", sdbusplus::vtable::property_::emits_change,
            [this](const int64_t& newVal, int64_t& oldVal) {
                oldVal = newVal;
                threshold = newVal;
                lg2::info("Threshold updated to {THRESHOLD}", "THRESHOLD",
                          threshold);
                return 1;
            },
            [this](const auto&) { return threshold; });

        iface->register_property_r<bool>(
            "AlertActive", sdbusplus::vtable::property_::emits_change,
            [this](const auto&) { return alertActive; });

        // Register ThresholdAlert signal
        iface->register_signal<bool, int64_t>("ThresholdAlert");

        iface->initialize();

        conn->request_name(serviceName);

        lg2::info("L2 Monitor service started - ID: 835051, Name: Sonny Chu");
        lg2::info("Monitoring file: {FILE}", "FILE", monitorFile);
        lg2::info("Default threshold: {THRESHOLD}", "THRESHOLD",
                  defaultThreshold);
    }

    void startMonitoring() //開始監控
    {
        timer.expires_after(pollInterval);
        timer.async_wait([this](const boost::system::error_code& ec) {
            if (!ec)
            {
                pollFile();
                startMonitoring();
            }
        });
    }

    void pollFile()  //讀取檔案
    {
        std::ifstream file(monitorFile);
        if (!file.is_open())
        {
            return;
        }

        int64_t newValue = 0;
        if (!(file >> newValue))
        {
            return;
        }

        if (newValue != currentValue)
        {
            currentValue = newValue;
            iface->signal_property("Value");
            lg2::info("Value updated to {VALUE}", "VALUE", currentValue);

            checkThreshold();
        }
    }

    void checkThreshold()  //檢查門檻
    {
        bool newAlertState = (currentValue > threshold);

        if (newAlertState != alertActive)
        {
            alertActive = newAlertState;
            iface->signal_property("AlertActive");

            auto signal = iface->new_signal("ThresholdAlert");
            signal.append(alertActive, currentValue);
            signal.signal_send();

            if (alertActive)
            {
                lg2::warning(
                    "ALERT: Value {VALUE} exceeded threshold {THRESHOLD}",
                    "VALUE", currentValue, "THRESHOLD", threshold);
            }
            else
            {
                lg2::info(
                    "ALERT CLEARED: Value {VALUE} below threshold {THRESHOLD}",
                    "VALUE", currentValue, "THRESHOLD", threshold);
            }
        }
    }

    boost::asio::io_context& io;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    sdbusplus::asio::object_server server;
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
    boost::asio::steady_timer timer;

    int64_t currentValue;
    int64_t threshold;
    bool alertActive;
};

} // namespace

int main()
{
    boost::asio::io_context io;  //建立io_context(調度中心，負責管理/監控所有事件)
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);  //連線到D-Bus，與其他程式溝通

    L2Monitor monitor(io, conn); //working

    io.run();  //io_context不斷運行

    return 0;
}
