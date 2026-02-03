#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <chrono>
#include <memory>
#include <string>

namespace sensor_monitor
{

/**
 * @class SensorMonitor
 * @brief D-Bus service that monitors a file value and sends threshold alarms
 *
 * Level 1: Provides D-Bus object path, interface, and properties
 * Level 2: Automatically monitors file values and updates properties
 * Level 3: Sends signal alarm when value exceeds critical threshold
 */
class SensorMonitor
{
  public:
    /**
     * @brief Construct a new SensorMonitor object
     *
     * @param bus D-Bus connection
     * @param objServer Object server for D-Bus interface
     * @param filePath Path to the file to monitor
     * @param criticalHigh Critical high threshold value
     * @param criticalLow Critical low threshold value
     * @param pollInterval Interval between file reads
     */
    SensorMonitor(sdbusplus::asio::connection& bus,
                  sdbusplus::asio::object_server& objServer,
                  const std::string& filePath, double criticalHigh,
                  double criticalLow, std::chrono::milliseconds pollInterval);

    ~SensorMonitor();

    // Delete copy constructor and assignment operator
    SensorMonitor(const SensorMonitor&) = delete;
    SensorMonitor& operator=(const SensorMonitor&) = delete;

    /**
     * @brief Start monitoring the file
     */
    void startMonitoring();

    /**
     * @brief Stop monitoring the file
     */
    void stopMonitoring();

    /**
     * @brief Get current sensor value
     * @return Current value
     */
    double getValue() const;

    /**
     * @brief Check if sensor is in alarm state
     * @return true if alarm is active
     */
    bool isAlarmActive() const;

  private:
    /**
     * @brief Read value from the monitored file
     * @return Value read from file, or NaN on error
     */
    double readFileValue();

    /**
     * @brief Update the sensor value and check thresholds
     * @param newValue New value to set
     */
    void updateValue(double newValue);

    /**
     * @brief Check thresholds and send alarm signal if needed
     * @param value Value to check against thresholds
     */
    void checkThresholds(double value);

    /**
     * @brief Timer callback for periodic monitoring
     */
    void monitorCallback();

    /**
     * @brief Send threshold alarm signal
     * @param alarmHigh true if high threshold exceeded
     * @param alarmLow true if low threshold exceeded
     */
    void sendAlarmSignal(bool alarmHigh, bool alarmLow);

    // D-Bus connection and server
    sdbusplus::asio::connection& bus;
    sdbusplus::asio::object_server& objServer;

    // D-Bus interfaces
    std::shared_ptr<sdbusplus::asio::dbus_interface> valueInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> thresholdInterface;

    // Configuration
    std::string filePath;
    double criticalHigh;
    double criticalLow;
    std::chrono::milliseconds pollInterval;

    // Current state
    double currentValue;
    bool alarmHighActive;
    bool alarmLowActive;
    bool monitoring;

    // Timer for periodic monitoring
    std::unique_ptr<boost::asio::steady_timer> monitorTimer;

    // D-Bus object path
    static constexpr const char* objectPath =
        "/xyz/openbmc_project/sensors/temperature/monitored_sensor";
};

} // namespace sensor_monitor
