/**
 * @file l4.cpp
 * @brief GPIO Monitor Service - L4 Training
 *
 * Monitors a GPIO input pin and outputs state changes.
 * Default: GPIOZ1 (line 201) - can be changed via environment variable.
 */

#include <gpiod.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/signal_set.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>

PHOSPHOR_LOG2_USING;

namespace
{

// Default GPIO configuration
// GPIOZ1 = line 201 (Z group starts at 200, line 1 = 201)
constexpr const char* defaultGpioChip = "gpiochip0";
constexpr unsigned int defaultGpioLine = 201;
constexpr const char* gpioConsumer = "l4-gpio-monitor";

class GpioMonitor
{
  public:
    GpioMonitor(boost::asio::io_context& io, const char* chipName,
                unsigned int lineOffset) :
        io(io), gpioEventDescriptor(io), chip(nullptr), line(nullptr)
    {
        chip = gpiod_chip_open_by_name(chipName);
        if (chip == nullptr)
        {
            error("Failed to open GPIO chip {CHIP}: {ERR}", "CHIP", chipName,
                  "ERR", std::strerror(errno));
            throw std::runtime_error("Failed to open GPIO chip");
        }

        line = gpiod_chip_get_line(chip, lineOffset);
        if (line == nullptr)
        {
            error("Failed to get GPIO line {LINE}: {ERR}", "LINE", lineOffset,
                  "ERR", std::strerror(errno));
            gpiod_chip_close(chip);
            throw std::runtime_error("Failed to get GPIO line");
        }

        // Request line as input with both edge detection
        int ret = gpiod_line_request_both_edges_events(line, gpioConsumer);
        if (ret < 0)
        {
            error("Failed to request GPIO line events: {ERR}", "ERR",
                  std::strerror(errno));
            gpiod_chip_close(chip);
            throw std::runtime_error("Failed to request GPIO line events");
        }

        // Get initial state
        int value = gpiod_line_get_value(line);
        if (value < 0)
        {
            warning("Failed to get initial GPIO value: {ERR}", "ERR",
                    std::strerror(errno));
        }
        else
        {
            info("GPIO Monitor started - Chip: {CHIP}, Line: {LINE}, "
                 "Initial state: {STATE}",
                 "CHIP", chipName, "LINE", lineOffset, "STATE",
                 value ? "HIGH" : "LOW");
        }

        // Get file descriptor for async monitoring
        int fd = gpiod_line_event_get_fd(line);
        if (fd < 0)
        {
            error("Failed to get GPIO event fd: {ERR}", "ERR",
                  std::strerror(errno));
            gpiod_line_release(line);
            gpiod_chip_close(chip);
            throw std::runtime_error("Failed to get GPIO event fd");
        }

        gpioEventDescriptor.assign(fd);
        waitForGpioEvent();
    }

    ~GpioMonitor()
    {
        if (line != nullptr)
        {
            gpiod_line_release(line);
        }
        if (chip != nullptr)
        {
            gpiod_chip_close(chip);
        }
    }

    // Disable copy
    GpioMonitor(const GpioMonitor&) = delete;
    GpioMonitor& operator=(const GpioMonitor&) = delete;

  private:
    void waitForGpioEvent()
    {
        gpioEventDescriptor.async_wait(
            boost::asio::posix::stream_descriptor::wait_read,
            [this](const boost::system::error_code& ec) {
                if (ec)
                {
                    if (ec != boost::asio::error::operation_aborted)
                    {
                        error("GPIO async_wait error: {ERR}", "ERR",
                              ec.message());
                    }
                    return;
                }
                handleGpioEvent();
            });
    }

    void handleGpioEvent()
    {
        gpiod_line_event event{};
        int ret = gpiod_line_event_read(line, &event);
        if (ret < 0)
        {
            error("Failed to read GPIO event: {ERR}", "ERR",
                  std::strerror(errno));
            waitForGpioEvent();
            return;
        }

        const char* eventType = "UNKNOWN";
        const char* newState = "UNKNOWN";

        if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
        {
            eventType = "RISING_EDGE";
            newState = "HIGH";
        }
        else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
        {
            eventType = "FALLING_EDGE";
            newState = "LOW";
        }

        info("GPIO state changed - Event: {EVENT}, New state: {STATE}, "
             "Timestamp: {TS}",
             "EVENT", eventType, "STATE", newState, "TS",
             event.ts.tv_sec * 1000000000ULL + event.ts.tv_nsec);

        waitForGpioEvent();
    }

    boost::asio::io_context& io;
    boost::asio::posix::stream_descriptor gpioEventDescriptor;
    gpiod_chip* chip;
    gpiod_line* line;
};

} // namespace

int main(int argc, char* argv[])
{
    info("L4 GPIO Monitor Service starting...");

    // Allow configuration via environment or command line
    const char* chipName = defaultGpioChip;
    unsigned int lineOffset = defaultGpioLine;

    // Check environment variables
    const char* envChip = std::getenv("GPIO_CHIP");
    const char* envLine = std::getenv("GPIO_LINE");

    if (envChip != nullptr)
    {
        chipName = envChip;
    }
    if (envLine != nullptr)
    {
        lineOffset = static_cast<unsigned int>(std::strtoul(envLine, nullptr, 10));
    }

    // Command line override: l4 <chip> <line>
    if (argc >= 3)
    {
        chipName = argv[1];
        lineOffset = static_cast<unsigned int>(std::strtoul(argv[2], nullptr, 10));
    }

    info("Configuration - Chip: {CHIP}, Line: {LINE}", "CHIP", chipName, "LINE",
         lineOffset);

    try
    {
        boost::asio::io_context io;

        // Setup signal handling for graceful shutdown
        boost::asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait(
            [&io](const boost::system::error_code& ec, int signum) {
                if (!ec)
                {
                    info("Received signal {SIG}, shutting down...", "SIG",
                         signum);
                    io.stop();
                }
            });

        GpioMonitor monitor(io, chipName, lineOffset);

        info("GPIO Monitor running. Waiting for GPIO events...");
        io.run();
    }
    catch (const std::exception& e)
    {
        error("Fatal error: {ERR}", "ERR", e.what());
        return 1;
    }

    info("L4 GPIO Monitor Service stopped.");
    return 0;
}
