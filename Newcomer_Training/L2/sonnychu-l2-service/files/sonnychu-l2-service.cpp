/**
 * SonnyChu L2 Training Service
 * 
 * Level 1: D-Bus Service with objectpath, interface, properties
 * Level 2: Auto monitor file value and display
 * Level 3: Send signal alarm when value exceeds critical value
 */

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

// D-Bus 命名常數
constexpr const char* SERVICE_NAME = "xyz.openbmc_project.SonnyChu.L2Service";
constexpr const char* OBJECT_PATH = "/xyz/openbmc_project/sonnychu/l2service";
constexpr const char* INTERFACE_NAME = "xyz.openbmc_project.SonnyChu.Value";

// 監控設定
constexpr const char* MONITOR_FILE = "/tmp/sensor_value";
constexpr int MONITOR_INTERVAL_SEC = 5;

// 全域變數
static double currentValue = 25.0;
static double criticalThreshold = 50.0;
static std::string description = "SonnyChu L2 Training Service";
static bool alarmActive = false;

// Interface 指標（用於發送 signal）
static std::shared_ptr<sdbusplus::asio::dbus_interface> valueInterface;

/**
 * Level 2: 讀取監控檔案的數值
 */
double readValueFromFile()
{
    std::ifstream file(MONITOR_FILE);
    if (file.is_open())
    {
        double value;
        if (file >> value)
        {
            std::cout << "[Level 2] Read value from file: " << value << std::endl;
            return value;
        }
    }
    return currentValue;
}

/**
 * Level 3: 發送 ThresholdAlarm Signal
 */
void sendThresholdAlarm(double value, double threshold, bool isAssert)
{
    if (valueInterface)
    {
        try
        {
            sdbusplus::message_t msg = valueInterface->new_signal("ThresholdAlarm");
            msg.append(value, threshold, isAssert);
            msg.signal_send();
            
            std::cout << "[Level 3] Signal sent - ThresholdAlarm("
                      << "value=" << value 
                      << ", threshold=" << threshold 
                      << ", isAssert=" << (isAssert ? "true" : "false") 
                      << ")" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[Level 3] Failed to send signal: " << e.what() << std::endl;
        }
    }
}

/**
 * Level 2 & 3: 定時監控檔案並檢查閾值
 */
void monitorFile(boost::asio::steady_timer& timer, 
                 std::shared_ptr<sdbusplus::asio::dbus_interface> iface)
{
    double newValue = readValueFromFile();
    
    if (newValue != currentValue)
    {
        currentValue = newValue;
        iface->set_property("Value", currentValue);
        std::cout << "[Level 2] Property updated: Value = " << currentValue << std::endl;
    }
    
    // Level 3: 檢查是否超過閾值
    if (currentValue > criticalThreshold && !alarmActive)
    {
        alarmActive = true;
        sendThresholdAlarm(currentValue, criticalThreshold, true);
    }
    else if (currentValue <= criticalThreshold && alarmActive)
    {
        alarmActive = false;
        sendThresholdAlarm(currentValue, criticalThreshold, false);
    }
    
    timer.expires_after(std::chrono::seconds(MONITOR_INTERVAL_SEC));
    timer.async_wait([&timer, iface](const boost::system::error_code& ec) {
        if (!ec)
        {
            monitorFile(timer, iface);
        }
    });
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  SonnyChu L2 Training Service" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Bus Name:    " << SERVICE_NAME << std::endl;
    std::cout << "Object Path: " << OBJECT_PATH << std::endl;
    std::cout << "Interface:   " << INTERFACE_NAME << std::endl;
    std::cout << "Monitor:     " << MONITOR_FILE << " (every " << MONITOR_INTERVAL_SEC << "s)" << std::endl;
    std::cout << "Threshold:   " << criticalThreshold << std::endl;
    std::cout << "========================================" << std::endl;

    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    
    conn->request_name(SERVICE_NAME);
    
    sdbusplus::asio::object_server server(conn);
    
    // Level 1: 創建 Interface 並註冊 Properties
    valueInterface = server.add_interface(OBJECT_PATH, INTERFACE_NAME);
    
    // Property: Value (可讀寫)
    valueInterface->register_property(
        "Value", currentValue,
        [](const double& newValue, double& value) {
            std::cout << "[Level 1] Property Value changed: " << value << " -> " << newValue << std::endl;
            value = newValue;
            return true;
        },
        [](const double& value) {
            return value;
        }
    );
    
    // Property: Description (唯讀)
    valueInterface->register_property_r(
        "Description", description,
        sdbusplus::vtable::property_::const_,
        [](const std::string& desc) {
            return desc;
        }
    );
    
    // Property: CriticalThreshold (可讀寫)
    valueInterface->register_property(
        "CriticalThreshold", criticalThreshold,
        [](const double& newValue, double& value) {
            std::cout << "[Level 1] CriticalThreshold changed: " << value << " -> " << newValue << std::endl;
            value = newValue;
            criticalThreshold = newValue;
            return true;
        },
        [](const double& value) {
            return value;
        }
    );
    
    // Level 3: 註冊 Signal
    valueInterface->register_signal<double, double, bool>("ThresholdAlarm");
    
    valueInterface->initialize();
    
    std::cout << "[Level 1] D-Bus Service initialized successfully!" << std::endl;
    
    // Level 2: 啟動檔案監控 每五秒讀取一次檔案，檢查數值是否超標！
    boost::asio::steady_timer monitorTimer(io);
    monitorTimer.expires_after(std::chrono::seconds(MONITOR_INTERVAL_SEC));
    monitorTimer.async_wait([&monitorTimer](const boost::system::error_code& ec) {
        if (!ec)
        {
            monitorFile(monitorTimer, valueInterface);
        }
    });
    
    std::cout << "[Level 2] File monitoring started!" << std::endl;
    std::cout << "[Level 3] ThresholdAlarm signal registered!" << std::endl;
    std::cout << "Service is running..." << std::endl;
    
    io.run();
    
    return 0;
}
