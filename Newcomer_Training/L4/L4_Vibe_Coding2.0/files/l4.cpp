#include <gpiod.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/signal_set.hpp>
#include <phosphor-logging/lg2.hpp>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <string>

PHOSPHOR_LOG2_USING;

namespace gpio_monitor
{

// GPIO configuration - use GPIOZ0 (line 200) which is typically free
constexpr const char* gpioChipName = "gpiochip0";
constexpr unsigned int gpioLineOffset = 200; // GPIOZ0
constexpr const char* gpioLineName = "GPIOZ0";

class GpioMonitor
{
  public:
    GpioMonitor(boost::asio::io_context& ioc) :
        ioc(ioc), gpioEventDescriptor(ioc), chip(nullptr), line(nullptr),
        simulatedState(0)
    {}

    ~GpioMonitor()
    {
        cleanup();
    }

    bool initialize()
    {
        // Open GPIO chip
        chip = gpiod_chip_open_by_name(gpioChipName);
        if (!chip)
        {
            error("Failed to open GPIO chip: {CHIP}", "CHIP", gpioChipName);
            return false;
        }
        info("Opened GPIO chip: {CHIP}", "CHIP", gpioChipName);

        // Get GPIO line
        line = gpiod_chip_get_line(chip, gpioLineOffset);
        if (!line)
        {
            error("Failed to get GPIO line {OFFSET}", "OFFSET", gpioLineOffset);
            return false;
        }
        info("Got GPIO line: {NAME} (offset {OFFSET})", "NAME", gpioLineName,
             "OFFSET", gpioLineOffset);

        // Request line for event monitoring (both edges)
        int ret = gpiod_line_request_both_edges_events(line, "l4-gpio-monitor");
        if (ret < 0)
        {
            error("Failed to request GPIO line for events: {NAME}", "NAME",
                  gpioLineName);
            return false;
        }
        info("Requested GPIO line for edge events: {NAME}", "NAME",
             gpioLineName);

        // Get initial state
        int value = gpiod_line_get_value(line);
        if (value >= 0)
        {
            simulatedState = value;
            info("Initial GPIO state: {NAME} = {VALUE}", "NAME", gpioLineName,
                 "VALUE", value);
        }

        // Get file descriptor for async monitoring
        int fd = gpiod_line_event_get_fd(line);
        if (fd < 0)
        {
            error("Failed to get GPIO event file descriptor");
            return false;
        }

        // Assign fd to boost::asio stream descriptor
        gpioEventDescriptor.assign(fd);

        return true;
    }

    void startMonitoring()
    {
        info("Starting GPIO monitoring for {NAME}", "NAME", gpioLineName);
        info("Send SIGUSR1 to simulate GPIO state change (kill -USR1 <pid>)");
        asyncWaitForEvent();
    }

    // Simulate GPIO state change (for QEMU testing)
    void simulateStateChange()
    {
        int oldState = simulatedState;
        simulatedState = (simulatedState == 0) ? 1 : 0;
        const char* eventType = (simulatedState == 1) ? "RISING" : "FALLING";

        info("GPIO state changed: {NAME} - Event: {EVENT}, Value: {VALUE}",
             "NAME", gpioLineName, "EVENT", eventType, "VALUE", simulatedState);
        info("(Simulated: {OLD} -> {NEW})", "OLD", oldState, "NEW",
             simulatedState);
    }

  private:
    void asyncWaitForEvent()
    {
        gpioEventDescriptor.async_wait(
            boost::asio::posix::stream_descriptor::wait_read,
            [this](const boost::system::error_code& ec) {
                handleGpioEvent(ec);
            });
    }

    void handleGpioEvent(const boost::system::error_code& ec)
    {
        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted)
            {
                info("GPIO monitoring stopped");
                return;
            }
            error("GPIO event wait error: {MSG}", "MSG", ec.message());
            return;
        }

        // Read the event
        struct gpiod_line_event event;
        int ret = gpiod_line_event_read(line, &event);
        if (ret < 0)
        {
            error("Failed to read GPIO event");
            asyncWaitForEvent();
            return;
        }

        // Determine event type
        const char* eventType =
            (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ? "RISING"
                                                               : "FALLING";

        // Get current value
        int currentValue = gpiod_line_get_value(line);
        simulatedState = currentValue;

        info("GPIO state changed: {NAME} - Event: {EVENT}, Value: {VALUE}",
             "NAME", gpioLineName, "EVENT", eventType, "VALUE", currentValue);

        // Continue monitoring
        asyncWaitForEvent();
    }

    void cleanup()
    {
        if (gpioEventDescriptor.is_open())
        {
            boost::system::error_code ec;
            gpioEventDescriptor.release();
        }

        if (line)
        {
            gpiod_line_release(line);
            line = nullptr;
        }

        if (chip)
        {
            gpiod_chip_close(chip);
            chip = nullptr;
        }
    }

    boost::asio::io_context& ioc;
    boost::asio::posix::stream_descriptor gpioEventDescriptor;
    struct gpiod_chip* chip;
    struct gpiod_line* line;
    int simulatedState;
};

} // namespace gpio_monitor

int main()
{
    info("L4 GPIO Monitor Service starting...");

    boost::asio::io_context ioc;

    // Create GPIO monitor
    gpio_monitor::GpioMonitor monitor(ioc);

    // Setup signal handling for graceful shutdown (SIGINT, SIGTERM)
    boost::asio::signal_set shutdownSignals(ioc, SIGINT, SIGTERM);
    shutdownSignals.async_wait(
        [&ioc](const boost::system::error_code&, int sigNum) {
            info("Received signal {SIG}, shutting down...", "SIG", sigNum);
            ioc.stop();
        });

    // Setup SIGUSR1 for simulating GPIO state change (for QEMU testing)
    boost::asio::signal_set simulateSignal(ioc, SIGUSR1);
    std::function<void(const boost::system::error_code&, int)> sigusr1Handler;
    sigusr1Handler = [&monitor, &simulateSignal,
                      &sigusr1Handler](const boost::system::error_code& ec,
                                       int) {
        if (!ec)
        {
            monitor.simulateStateChange();
            // Re-register for next signal
            simulateSignal.async_wait(sigusr1Handler);
        }
    };
    simulateSignal.async_wait(sigusr1Handler);

    // Initialize GPIO monitor
    if (!monitor.initialize())
    {
        error("Failed to initialize GPIO monitor");
        return EXIT_FAILURE;
    }

    monitor.startMonitoring();

    info("L4 GPIO Monitor Service running");

    // Run the event loop
    ioc.run();

    info("L4 GPIO Monitor Service stopped");
    return EXIT_SUCCESS;
}
