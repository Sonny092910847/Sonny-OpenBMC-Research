// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <chrono>
#include <fstream>
#include <memory>
#include <string>

// D-Bus naming
constexpr auto serviceName = "xyz.openbmc_project.Training.L2";
constexpr auto objectPath = "/xyz/openbmc_project/training/l2";
constexpr auto interfaceName = "xyz.openbmc_project.Training.L2.Monitor";

// Monitor settings
constexpr auto monitorFile = "/tmp/l2_monitor_value";
constexpr int64_t defaultThreshold = 50;
constexpr auto pollIntervalSeconds = 3;

class L2Monitor
{
  public:
    L2Monitor(boost::asio::io_context& io,
              std::shared_ptr<sdbusplus::asio::connection> conn) :
        io(io), conn(conn), pollTimer(io), value(0), threshold(defaultThreshold),
        alertActive(false)
    {
        server = std::make_unique<sdbusplus::asio::object_server>(conn);
        setupDbusInterface();
        startPolling();
        lg2::info("L2 Monitor started - ID: 835051, Name: Sonny Chu");
    }

  private:
    void setupDbusInterface()
    {
        iface = server->add_interface(objectPath, interfaceName);

        // Register Value property (read-only, updated by file monitor)
        iface->register_property_r<int64_t>(
            "Value", sdbusplus::vtable::property_::emits_change,
            [this](const auto&) { return value; });

        // Register Threshold property (read-write)
        iface->register_property_rw<int64_t>(
            "Threshold", sdbusplus::vtable::property_::emits_change,
            [this](const int64_t& newValue, int64_t&) {
                threshold = newValue;
                lg2::info("Threshold updated to {THRESHOLD}", "THRESHOLD",
                          threshold);
                return true;
            },
            [this](const auto&) { return threshold; });

        // Register AlertActive property (read-only)
        iface->register_property_r<bool>(
            "AlertActive", sdbusplus::vtable::property_::emits_change,
            [this](const auto&) { return alertActive; });

        // Register ThresholdAlert signal
        iface->register_signal<bool, int64_t>("ThresholdAlert");

        iface->initialize();
    }

    void startPolling()
    {
        pollTimer.expires_after(std::chrono::seconds(pollIntervalSeconds));
        pollTimer.async_wait([this](const boost::system::error_code& ec) {
            if (!ec)
            {
                pollFile();
                startPolling();
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
            lg2::warning("Failed to read value from {FILE}", "FILE",
                         monitorFile);
            return;
        }

        if (newValue != value)
        {
            value = newValue;
            lg2::info("Value updated to {VALUE}", "VALUE", value);
            iface->signal_property("Value");

            checkThreshold();
        }
    }

    void checkThreshold()
    {
        bool shouldAlert = (value > threshold);

        if (shouldAlert != alertActive)
        {
            alertActive = shouldAlert;
            iface->signal_property("AlertActive");

            // Emit ThresholdAlert signal
            auto msg = iface->new_signal("ThresholdAlert");
            msg.append(alertActive, value);
            msg.signal_send();

            if (alertActive)
            {
                lg2::warning(
                    "ALERT: Value {VALUE} exceeded threshold {THRESHOLD}",
                    "VALUE", value, "THRESHOLD", threshold);
            }
            else
            {
                lg2::info(
                    "ALERT CLEARED: Value {VALUE} is below threshold {THRESHOLD}",
                    "VALUE", value, "THRESHOLD", threshold);
            }
        }
    }

    boost::asio::io_context& io;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    std::unique_ptr<sdbusplus::asio::object_server> server;
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
    boost::asio::steady_timer pollTimer;

    int64_t value;
    int64_t threshold;
    bool alertActive;
};

int main()
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    conn->request_name(serviceName);

    L2Monitor monitor(io, conn);

    io.run();

    return 0;
}
