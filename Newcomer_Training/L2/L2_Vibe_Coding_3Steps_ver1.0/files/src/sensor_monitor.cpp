#include "sensor_monitor.hpp"

#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>

namespace sensor_monitor
{

SensorMonitor::SensorMonitor(sdbusplus::asio::connection& bus,
                             sdbusplus::asio::object_server& objServer,
                             const std::string& filePath, double criticalHigh,
                             double criticalLow,
                             std::chrono::milliseconds pollInterval) :
    bus(bus), objServer(objServer), filePath(filePath),
    criticalHigh(criticalHigh), criticalLow(criticalLow),
    pollInterval(pollInterval), currentValue(0.0), alarmHighActive(false),
    alarmLowActive(false), monitoring(false)
{
    std::cerr << "[SensorMonitor] Creating D-Bus interfaces..." << std::endl;

    // Level 1: Create D-Bus interface with properties

    // Create the Value interface (xyz.openbmc_project.Sensor.Value)
    std::cerr << "[SensorMonitor] Adding interface: "
              << "xyz.openbmc_project.Sensor.Value" << std::endl;
    valueInterface = objServer.add_interface(
        objectPath, "xyz.openbmc_project.Sensor.Value");

    // Register Value property
    valueInterface->register_property(
        "Value", currentValue,
        // Setter
        [this](const double& newValue, double& oldValue) {
            oldValue = newValue;
            this->currentValue = newValue;
            return true;
        },
        // Getter
        [this](const double& /* unused */) { return this->currentValue; });

    // Register Unit property
    valueInterface->register_property("Unit", std::string("DegreesC"));

    // Register MaxValue property
    valueInterface->register_property("MaxValue", criticalHigh);

    // Register MinValue property
    valueInterface->register_property("MinValue", criticalLow);

    valueInterface->initialize();
    std::cerr << "[SensorMonitor] Value interface initialized" << std::endl;

    // Level 3: Create Threshold interface for alarm signals
    std::cerr << "[SensorMonitor] Adding interface: "
              << "xyz.openbmc_project.Sensor.Threshold.Critical" << std::endl;
    thresholdInterface = objServer.add_interface(
        objectPath, "xyz.openbmc_project.Sensor.Threshold.Critical");

    // Register threshold properties
    thresholdInterface->register_property("CriticalHigh", criticalHigh);
    thresholdInterface->register_property("CriticalLow", criticalLow);
    thresholdInterface->register_property(
        "CriticalAlarmHigh", alarmHighActive,
        sdbusplus::asio::PropertyPermission::readOnly);
    thresholdInterface->register_property(
        "CriticalAlarmLow", alarmLowActive,
        sdbusplus::asio::PropertyPermission::readOnly);

    thresholdInterface->initialize();
    std::cerr << "[SensorMonitor] Threshold interface initialized" << std::endl;

    std::cerr << "[SensorMonitor] Initialized successfully" << std::endl;
    std::cerr << "[SensorMonitor] Object path: " << objectPath << std::endl;
    std::cerr << "[SensorMonitor] Monitoring file: " << filePath << std::endl;
}

SensorMonitor::~SensorMonitor()
{
    std::cerr << "[SensorMonitor] Destructor called" << std::endl;
    stopMonitoring();

    if (valueInterface)
    {
        objServer.remove_interface(valueInterface);
    }
    if (thresholdInterface)
    {
        objServer.remove_interface(thresholdInterface);
    }
}

void SensorMonitor::startMonitoring()
{
    if (monitoring)
    {
        std::cerr << "[SensorMonitor] Already monitoring" << std::endl;
        return;
    }

    monitoring = true;

    // Create timer for periodic monitoring
    monitorTimer =
        std::make_unique<boost::asio::steady_timer>(bus.get_io_context());

    std::cerr << "[SensorMonitor] Starting file monitoring, interval="
              << pollInterval.count() << "ms" << std::endl;

    // Start the monitoring loop
    monitorCallback();
}

void SensorMonitor::stopMonitoring()
{
    monitoring = false;

    if (monitorTimer)
    {
        monitorTimer->cancel();
        monitorTimer.reset();
    }

    std::cerr << "[SensorMonitor] Stopped file monitoring" << std::endl;
}

double SensorMonitor::getValue() const
{
    return currentValue;
}

bool SensorMonitor::isAlarmActive() const
{
    return alarmHighActive || alarmLowActive;
}

double SensorMonitor::readFileValue()
{
    // Level 2: Read value from monitored file
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[SensorMonitor] Failed to open file: " << filePath
                  << std::endl;
        return std::nan("");
    }

    double value = 0.0;
    if (!(file >> value))
    {
        std::cerr << "[SensorMonitor] Failed to read value from file: "
                  << filePath << std::endl;
        return std::nan("");
    }

    return value;
}

void SensorMonitor::updateValue(double newValue)
{
    if (std::isnan(newValue))
    {
        return;
    }

    currentValue = newValue;

    // Update D-Bus property
    valueInterface->set_property("Value", currentValue);

    std::cerr << "[SensorMonitor] Value updated: " << currentValue << std::endl;

    // Level 3: Check thresholds
    checkThresholds(newValue);
}

void SensorMonitor::checkThresholds(double value)
{
    bool prevAlarmHigh = alarmHighActive;
    bool prevAlarmLow = alarmLowActive;

    // Check high threshold
    alarmHighActive = (value >= criticalHigh);

    // Check low threshold
    alarmLowActive = (value <= criticalLow);

    // Send signal if alarm state changed
    if (alarmHighActive != prevAlarmHigh || alarmLowActive != prevAlarmLow)
    {
        sendAlarmSignal(alarmHighActive, alarmLowActive);

        // Update D-Bus properties
        thresholdInterface->set_property("CriticalAlarmHigh", alarmHighActive);
        thresholdInterface->set_property("CriticalAlarmLow", alarmLowActive);
    }
}

void SensorMonitor::sendAlarmSignal(bool alarmHigh, bool alarmLow)
{
    // Level 3: Send threshold alarm signal
    if (alarmHigh)
    {
        std::cerr << "[SensorMonitor] ALARM: Value " << currentValue
                  << " exceeded critical high threshold " << criticalHigh
                  << std::endl;

        // Send D-Bus signal for high alarm
        auto msg =
            bus.new_signal(objectPath,
                           "xyz.openbmc_project.Sensor.Threshold.Critical",
                           "CriticalHighAlarm");
        msg.append(currentValue, criticalHigh);
        msg.signal_send();
    }

    if (alarmLow)
    {
        std::cerr << "[SensorMonitor] ALARM: Value " << currentValue
                  << " below critical low threshold " << criticalLow
                  << std::endl;

        // Send D-Bus signal for low alarm
        auto msg =
            bus.new_signal(objectPath,
                           "xyz.openbmc_project.Sensor.Threshold.Critical",
                           "CriticalLowAlarm");
        msg.append(currentValue, criticalLow);
        msg.signal_send();
    }

    if (!alarmHigh && !alarmLow)
    {
        std::cerr << "[SensorMonitor] Alarm cleared: value " << currentValue
                  << " within normal range" << std::endl;
    }
}

void SensorMonitor::monitorCallback()
{
    if (!monitoring)
    {
        return;
    }

    // Level 2: Read and display the file value
    double value = readFileValue();
    updateValue(value);

    // Schedule next read
    monitorTimer->expires_after(pollInterval);
    monitorTimer->async_wait([this](const boost::system::error_code& ec) {
        if (!ec)
        {
            this->monitorCallback();
        }
    });
}

} // namespace sensor_monitor
