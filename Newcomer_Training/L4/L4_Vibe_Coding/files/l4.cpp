// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include <gpiod.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <memory>
#include <string>

PHOSPHOR_LOG2_USING;

namespace
{

constexpr auto serviceName = "xyz.openbmc_project.Training.L4";
constexpr auto objectPath = "/xyz/openbmc_project/training/l4";
constexpr auto interfaceName = "xyz.openbmc_project.Training.L4.GpioMonitor";

// GPIO configuration
// AST2600: GPIOA = GPIO 0-7, GPIOB = GPIO 8-15
// Using GPIOB0 (line 8) which is unused in QEMU for testing
constexpr auto gpioChipId = "gpiochip0";
constexpr int gpioLineNum = 8; // GPIOB0 (unused in QEMU)
constexpr auto gpioLineName = "GPIOB0";

class GpioMonitor
{
  public:
    GpioMonitor(boost::asio::io_context& io,
                std::shared_ptr<sdbusplus::asio::connection> conn) :
        io(io),
        conn(conn), server(conn), gpioEventDescriptor(io), gpioLine(nullptr),
        currentState(false)
    {
        setupDbusInterface();

        if (initGpio())
        {
            scheduleEventHandler();
        }
    }

    ~GpioMonitor()
    {
        if (gpioLine != nullptr)
        {
            gpiod_line_release(gpioLine);
        }
    }

  private:
    void setupDbusInterface()
    {
        iface = server.add_interface(objectPath, interfaceName);

        iface->register_property_r<bool>(
            "GpioState", sdbusplus::vtable::property_::emits_change,
            [this](const auto&) { return currentState; });

        iface->register_property_r<std::string>(
            "GpioName", sdbusplus::vtable::property_::const_,
            [](const auto&) { return std::string(gpioLineName); });

        iface->register_signal<bool>("GpioStateChanged");

        iface->initialize();

        conn->request_name(serviceName);

        lg2::info("L4 GPIO Monitor service started - ID: 835051, Name: Sonny "
                  "Chu");
        lg2::info("Monitoring {GPIO} (chip: {CHIP}, line: {LINE})", "GPIO",
                  gpioLineName, "CHIP", gpioChipId, "LINE", gpioLineNum);
    }

    bool initGpio()
    {
        // Open GPIO chip
        gpiod_chip* chip = gpiod_chip_open_by_name(gpioChipId);
        if (chip == nullptr)
        {
            lg2::error("Failed to open GPIO chip {CHIP}", "CHIP", gpioChipId);
            return false;
        }

        // Get GPIO line
        gpioLine = gpiod_chip_get_line(chip, gpioLineNum);
        if (gpioLine == nullptr)
        {
            lg2::error("Failed to get GPIO line {LINE}", "LINE", gpioLineNum);
            gpiod_chip_close(chip);
            return false;
        }

        // Request GPIO line for event monitoring (both edges)
        gpiod_line_request_config config{};
        config.consumer = "l4-gpio-monitor";
        config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
        config.flags = 0;

        if (gpiod_line_request(gpioLine, &config, 0) < 0)
        {
            lg2::error("Failed to request GPIO line {GPIO}", "GPIO",
                       gpioLineName);
            return false;
        }

        // Get initial state
        int value = gpiod_line_get_value(gpioLine);
        if (value >= 0)
        {
            currentState = (value == 1);
            lg2::info("Initial {GPIO} state: {STATE}", "GPIO", gpioLineName,
                      "STATE", currentState ? "HIGH" : "LOW");
        }

        // Get file descriptor for async monitoring
        int fd = gpiod_line_event_get_fd(gpioLine);
        if (fd < 0)
        {
            lg2::error("Failed to get event fd for {GPIO}", "GPIO",
                       gpioLineName);
            return false;
        }

        // Assign fd to boost::asio stream descriptor
        gpioEventDescriptor.assign(fd);

        lg2::info("{GPIO} monitoring initialized successfully", "GPIO",
                  gpioLineName);
        return true;
    }

    void scheduleEventHandler()
    {
        gpioEventDescriptor.async_wait(
            boost::asio::posix::stream_descriptor::wait_read,
            [this](const boost::system::error_code& ec) {
                if (ec)
                {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        return;
                    }
                    lg2::error("{GPIO} event handler error: {ERROR}", "GPIO",
                               gpioLineName, "ERROR", ec.message());
                    return;
                }
                handleGpioEvent();
            });
    }

    void handleGpioEvent()
    {
        gpiod_line_event gpioLineEvent{};

        if (gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                     &gpioLineEvent) < 0)
        {
            lg2::error("Failed to read {GPIO} event", "GPIO", gpioLineName);
            scheduleEventHandler();
            return;
        }

        bool newState =
            (gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE);

        if (newState != currentState)
        {
            currentState = newState;

            // Update D-Bus property
            iface->signal_property("GpioState");

            // Send D-Bus signal
            auto signal = iface->new_signal("GpioStateChanged");
            signal.append(currentState);
            signal.signal_send();

            // Log state change
            lg2::info("{GPIO} state changed to {STATE}", "GPIO", gpioLineName,
                      "STATE", currentState ? "HIGH" : "LOW");
        }

        // Continue monitoring
        scheduleEventHandler();
    }

    boost::asio::io_context& io;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    sdbusplus::asio::object_server server;
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
    boost::asio::posix::stream_descriptor gpioEventDescriptor;

    gpiod_line* gpioLine;
    bool currentState;
};

} // namespace

int main()
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    GpioMonitor monitor(io, conn);

    io.run();

    return 0;
}
