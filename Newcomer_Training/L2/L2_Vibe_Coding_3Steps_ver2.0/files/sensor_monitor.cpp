/**
 * @file sensor_monitor.cpp
 * @brief OpenBMC Sensor Monitor Service
 *
 * This service implements a D-Bus sensor monitoring service that:
 * - Level 1: Exposes D-Bus interfaces for sensor value and thresholds
 * - Level 2: Monitors /tmp/sensor_value file and updates D-Bus properties
 * - Level 3: Triggers threshold alarms when value exceeds critical limits
 */

#include <sys/inotify.h>
#include <unistd.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

PHOSPHOR_LOG2_USING;

namespace
{
// D-Bus configuration
constexpr auto serviceName = "xyz.openbmc_project.SensorMonitor";
constexpr auto objectPath =
    "/xyz/openbmc_project/sensors/temperature/monitored_sensor";
constexpr auto valueInterface = "xyz.openbmc_project.Sensor.Value";
constexpr auto thresholdInterface =
    "xyz.openbmc_project.Sensor.Threshold.Critical";

// Sensor configuration
constexpr auto sensorFilePath = "/tmp/sensor_value";
constexpr auto sensorUnit =
    "xyz.openbmc_project.Sensor.Value.Unit.DegreesC";
constexpr double defaultValue = 25.0;
constexpr double maxValue = 100.0;
constexpr double minValue = 0.0;
constexpr double criticalHigh = 80.0;
constexpr double criticalLow = 5.0;

// Inotify buffer size
constexpr size_t inotifyBufferSize = 1024;
} // namespace

class SensorMonitor : public std::enable_shared_from_this<SensorMonitor>
{
  public:
    SensorMonitor(boost::asio::io_context& io,
                  sdbusplus::asio::object_server& objectServer) :
        io_(io),
        objectServer_(objectServer), inotifyFd_(-1), watchDescriptor_(-1),
        inotifyStream_(io), retryTimer_(io), value_(defaultValue),
        criticalAlarmHigh_(false), criticalAlarmLow_(false)
    {}

    ~SensorMonitor()
    {
        cleanup();
    }

    bool initialize()
    {
        if (!createInterfaces())
        {
            error("Failed to create D-Bus interfaces");
            return false;
        }

        // Read initial value from file if it exists
        readSensorFile();

        // Setup inotify monitoring
        setupInotifyMonitoring();

        info("Sensor monitor initialized successfully");
        return true;
    }

  private:
    bool createInterfaces()
    {
        // Create Sensor.Value interface
        valueIface_ =
            objectServer_.add_interface(objectPath, valueInterface);

        if (!valueIface_)
        {
            error("Failed to create Value interface");
            return false;
        }

        // Register Value property (read-write to allow external updates)
        valueIface_->register_property(
            "Value", value_,
            // Setter
            [this](const double& newValue, double& oldValue) {
                if (newValue != oldValue)
                {
                    oldValue = newValue;
                    value_ = newValue;
                    updateThresholdAlarms();
                }
                return true;
            },
            // Getter
            [this](const double&) { return value_; });

        // Register Unit property (read-only)
        valueIface_->register_property(
            "Unit", std::string(sensorUnit),
            sdbusplus::asio::PropertyPermission::readOnly);

        // Register MaxValue property (read-only)
        valueIface_->register_property(
            "MaxValue", maxValue,
            sdbusplus::asio::PropertyPermission::readOnly);

        // Register MinValue property (read-only)
        valueIface_->register_property(
            "MinValue", minValue,
            sdbusplus::asio::PropertyPermission::readOnly);

        valueIface_->initialize();

        // Create Threshold.Critical interface
        thresholdIface_ =
            objectServer_.add_interface(objectPath, thresholdInterface);

        if (!thresholdIface_)
        {
            error("Failed to create Threshold interface");
            return false;
        }

        // Register CriticalHigh property (read-only)
        thresholdIface_->register_property(
            "CriticalHigh", criticalHigh,
            sdbusplus::asio::PropertyPermission::readOnly);

        // Register CriticalLow property (read-only)
        thresholdIface_->register_property(
            "CriticalLow", criticalLow,
            sdbusplus::asio::PropertyPermission::readOnly);

        // Register CriticalAlarmHigh property (read-only, updated by monitor)
        thresholdIface_->register_property(
            "CriticalAlarmHigh", criticalAlarmHigh_,
            sdbusplus::asio::PropertyPermission::readOnly);

        // Register CriticalAlarmLow property (read-only, updated by monitor)
        thresholdIface_->register_property(
            "CriticalAlarmLow", criticalAlarmLow_,
            sdbusplus::asio::PropertyPermission::readOnly);

        thresholdIface_->initialize();

        info("D-Bus interfaces created: {IFACE1}, {IFACE2}", "IFACE1",
             valueInterface, "IFACE2", thresholdInterface);
        return true;
    }

    void readSensorFile()
    {
        if (!std::filesystem::exists(sensorFilePath))
        {
            debug("Sensor file does not exist yet: {PATH}", "PATH",
                  sensorFilePath);
            return;
        }

        std::ifstream file(sensorFilePath);
        if (!file.is_open())
        {
            warning("Failed to open sensor file: {PATH}", "PATH",
                    sensorFilePath);
            return;
        }

        double newValue = 0.0;
        if (file >> newValue)
        {
            if (newValue != value_)
            {
                info("Sensor value updated: {OLD} -> {NEW}", "OLD", value_,
                     "NEW", newValue);
                value_ = newValue;
                valueIface_->set_property("Value", value_);
                updateThresholdAlarms();
            }
        }
        else
        {
            warning("Failed to parse sensor value from file");
        }
    }

    void updateThresholdAlarms()
    {
        bool newAlarmHigh = (value_ >= criticalHigh);
        bool newAlarmLow = (value_ <= criticalLow);

        if (newAlarmHigh != criticalAlarmHigh_)
        {
            criticalAlarmHigh_ = newAlarmHigh;
            thresholdIface_->set_property("CriticalAlarmHigh",
                                          criticalAlarmHigh_);
            if (criticalAlarmHigh_)
            {
                warning("Critical high alarm triggered: value={VALUE}, "
                        "threshold={THRESHOLD}",
                        "VALUE", value_, "THRESHOLD", criticalHigh);
            }
            else
            {
                info("Critical high alarm cleared");
            }
        }

        if (newAlarmLow != criticalAlarmLow_)
        {
            criticalAlarmLow_ = newAlarmLow;
            thresholdIface_->set_property("CriticalAlarmLow", criticalAlarmLow_);
            if (criticalAlarmLow_)
            {
                warning(
                    "Critical low alarm triggered: value={VALUE}, "
                    "threshold={THRESHOLD}",
                    "VALUE", value_, "THRESHOLD", criticalLow);
            }
            else
            {
                info("Critical low alarm cleared");
            }
        }
    }

    void setupInotifyMonitoring()
    {
        // Initialize inotify
        inotifyFd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (inotifyFd_ < 0)
        {
            error("Failed to initialize inotify: {ERRNO}", "ERRNO", errno);
            scheduleRetry();
            return;
        }

        // Watch the directory containing the sensor file
        std::filesystem::path filePath(sensorFilePath);
        std::string dirPath = filePath.parent_path().string();

        // Ensure /tmp exists (it should always exist)
        if (!std::filesystem::exists(dirPath))
        {
            error("Directory does not exist: {PATH}", "PATH", dirPath);
            close(inotifyFd_);
            inotifyFd_ = -1;
            scheduleRetry();
            return;
        }

        // Watch for file creation, modification, and close-write events
        watchDescriptor_ = inotify_add_watch(
            inotifyFd_, dirPath.c_str(),
            IN_CREATE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO);

        if (watchDescriptor_ < 0)
        {
            error("Failed to add inotify watch: {ERRNO}", "ERRNO", errno);
            close(inotifyFd_);
            inotifyFd_ = -1;
            scheduleRetry();
            return;
        }

        // Assign file descriptor to stream
        inotifyStream_.assign(inotifyFd_);

        info("Inotify monitoring started for: {PATH}", "PATH", dirPath);
        startAsyncRead();
    }

    void startAsyncRead()
    {
        auto self = shared_from_this();
        inotifyStream_.async_wait(
            boost::asio::posix::stream_descriptor::wait_read,
            [self](const boost::system::error_code& ec) {
                self->handleInotifyEvent(ec);
            });
    }

    void handleInotifyEvent(const boost::system::error_code& ec)
    {
        if (ec)
        {
            if (ec != boost::asio::error::operation_aborted)
            {
                error("Inotify async_wait error: {MSG}", "MSG", ec.message());
                cleanup();
                scheduleRetry();
            }
            return;
        }

        // Read inotify events
        std::array<char, inotifyBufferSize> buffer{};
        ssize_t length = read(inotifyFd_, buffer.data(), buffer.size());

        if (length < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                error("Failed to read inotify events: {ERRNO}", "ERRNO", errno);
            }
            startAsyncRead();
            return;
        }

        // Process events
        std::filesystem::path targetFile(sensorFilePath);
        std::string targetFilename = targetFile.filename().string();

        ssize_t i = 0;
        while (i < length)
        {
            auto* event = reinterpret_cast<inotify_event*>(&buffer[i]);

            if (event->len > 0)
            {
                std::string eventFilename(event->name);
                if (eventFilename == targetFilename)
                {
                    debug("File event detected: {FILE}, mask={MASK}", "FILE",
                          eventFilename, "MASK", event->mask);
                    readSensorFile();
                }
            }

            i += sizeof(inotify_event) + event->len;
        }

        startAsyncRead();
    }

    void scheduleRetry()
    {
        auto self = shared_from_this();
        retryTimer_.expires_after(std::chrono::seconds(5));
        retryTimer_.async_wait([self](const boost::system::error_code& ec) {
            if (!ec)
            {
                info("Retrying inotify setup...");
                self->setupInotifyMonitoring();
            }
        });
    }

    void cleanup()
    {
        retryTimer_.cancel();

        if (inotifyStream_.is_open())
        {
            boost::system::error_code ec;
            inotifyStream_.cancel(ec);
            inotifyStream_.release();
        }

        if (watchDescriptor_ >= 0 && inotifyFd_ >= 0)
        {
            inotify_rm_watch(inotifyFd_, watchDescriptor_);
            watchDescriptor_ = -1;
        }

        if (inotifyFd_ >= 0)
        {
            close(inotifyFd_);
            inotifyFd_ = -1;
        }
    }

    boost::asio::io_context& io_;
    sdbusplus::asio::object_server& objectServer_;

    int inotifyFd_;
    int watchDescriptor_;
    boost::asio::posix::stream_descriptor inotifyStream_;
    boost::asio::steady_timer retryTimer_;

    double value_;
    bool criticalAlarmHigh_;
    bool criticalAlarmLow_;

    std::shared_ptr<sdbusplus::asio::dbus_interface> valueIface_;
    std::shared_ptr<sdbusplus::asio::dbus_interface> thresholdIface_;
};

int main()
{
    info("Starting Sensor Monitor Service");

    boost::asio::io_context io;

    // Create D-Bus connection
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // Request the service name
    conn->request_name(serviceName);
    info("Registered D-Bus service: {NAME}", "NAME", serviceName);

    // Create object server
    sdbusplus::asio::object_server objectServer(conn);

    // Create sensor monitor
    auto monitor = std::make_shared<SensorMonitor>(io, objectServer);

    if (!monitor->initialize())
    {
        error("Failed to initialize sensor monitor");
        return EXIT_FAILURE;
    }

    info("Sensor Monitor Service running");
    io.run();

    return EXIT_SUCCESS;
}
