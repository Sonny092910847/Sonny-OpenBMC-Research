// SPDX-License-Identifier: Apache-2.0
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

PHOSPHOR_LOG2_USING;

namespace
{
// D-Bus naming
constexpr auto serviceName = "xyz.openbmc_project.Training.L2";
constexpr auto objectPath = "/xyz/openbmc_project/training/l2";
constexpr auto interfaceName = "xyz.openbmc_project.Training.L2.Monitor";

// Monitor settings
constexpr auto monitorFile = "/tmp/l2_monitor_value";
constexpr int64_t defaultThreshold = 50;
constexpr auto pollInterval = std::chrono::seconds(2);

class L2Monitor
{
  public:
    L2Monitor(boost::asio::io_context& io,
              std::shared_ptr<sdbusplus::asio::connection> conn) :
        io(io),                                                   //Initialize: io members, conn members, server members, timer members
        conn(conn), server(conn), timer(io), currentValue(0),     //Initialize: Initialize currentValue = 0, Initialize threshold = 50, Initialize alertActive = false
        threshold(defaultThreshold), alertActive(false)
    {
        setupDbusInterface();
        startMonitoring();
    }

  private:
    void setupDbusInterface()
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

    void startMonitoring()
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

    void pollFile()
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

    void checkThreshold()
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
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    L2Monitor monitor(io, conn);

    io.run();

    return 0;
}
