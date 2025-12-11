#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

static constexpr auto serviceName = "xyz.openbmc_project.TEST_Temp";
static constexpr auto objectPath = "/xyz/openbmc_project/sensors/temperature/TEST_Temp";
static constexpr auto valueInterface = "xyz.openbmc_project.Sensor.Value";
static constexpr auto warningInterface = "xyz.openbmc_project.Sensor.Threshold.Warning";
static constexpr auto criticalInterface = "xyz.openbmc_project.Sensor.Threshold.Critical";
static constexpr auto availabilityInterface = "xyz.openbmc_project.State.Decorator.Availability";
static constexpr auto operationalInterface = "xyz.openbmc_project.State.Decorator.OperationalStatus";

static constexpr auto tempFilePath = "/tmp/test/test_temp";
static constexpr double warningHigh = 80.0;
static constexpr double criticalHigh = 90.0;

bool warningAlarmHigh = false;
bool criticalAlarmHigh = false;
bool sensorAvailable = true; //new: 追蹤sensor可用狀態

static constexpr auto selLoggerService = "xyz.openbmc_project.Logging.IPMI";
static constexpr auto selLoggerPath = "/xyz/openbmc_project/Logging/IPMI";
static constexpr auto selLoggerInterface = "xyz.openbmc_project.Logging.IPMI";

void addSelEntry(sdbusplus::asio::connection& conn, const std::string& message,
                 const std::string& path, const std::vector<uint8_t>& eventData,
                 bool assert, uint16_t genId)
{
    try
    {
        auto method = conn.new_method_call(selLoggerService, selLoggerPath,
                                           selLoggerInterface, "IpmiSelAdd");
        method.append(message, path, eventData, assert, genId);
        conn.call_noreply(method);
        std::cout << "SEL entry added: " << message << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to add SEL entry: " << e.what() << std::endl;
    }
}

double readTemperature()
{
    std::ifstream file(tempFilePath);
    if (!file.is_open())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double temp;
    file >> temp;
    return temp;
}

void checkThresholds(double value,
                     std::shared_ptr<sdbusplus::asio::dbus_interface> warningIface,
                     std::shared_ptr<sdbusplus::asio::dbus_interface> criticalIface,
                     std::shared_ptr<sdbusplus::asio::connection> conn)
{
    bool newWarningAlarm = (value >= warningHigh);
    if (newWarningAlarm != warningAlarmHigh)
    {
        warningAlarmHigh = newWarningAlarm;
        warningIface->set_property("WarningAlarmHigh", warningAlarmHigh);
        std::cout << "Warning alarm changed to: " << (warningAlarmHigh ? "true" : "false") 
                  << " (value=" << value << ")" << std::endl;
        
        std::vector<uint8_t> eventData = {0x01, 0x07, static_cast<uint8_t>(value)};
        addSelEntry(*conn, "TEST_Temp Warning Threshold", objectPath, eventData, warningAlarmHigh, 0x2000);
    }
    
    bool newCriticalAlarm = (value >= criticalHigh);
    if (newCriticalAlarm != criticalAlarmHigh)
    {
        criticalAlarmHigh = newCriticalAlarm;
        criticalIface->set_property("CriticalAlarmHigh", criticalAlarmHigh);
        std::cout << "Critical alarm changed to: " << (criticalAlarmHigh ? "true" : "false")
                  << " (value=" << value << ")" << std::endl;
        
        std::vector<uint8_t> eventData = {0x01, 0x09, static_cast<uint8_t>(value)};
        addSelEntry(*conn, "TEST_Temp Critical Threshold", objectPath, eventData, criticalAlarmHigh, 0x2000);
    }
}

// v2.0_advance: 不自動建立檔案
int main()
{
    std::cout << "TEST_Temp Sensor Service starting..." << std::endl;
    std::cout << "File detection enabled - monitoring: " << tempFilePath << std::endl; 
    
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    conn->request_name(serviceName);
    
    sdbusplus::asio::object_server objectServer(conn);
    
    auto valueIface = objectServer.add_interface(objectPath, valueInterface);
    valueIface->register_property("Value", std::numeric_limits<double>::quiet_NaN(), //初始值是NaN，表示「尚未讀取」
        sdbusplus::asio::PropertyPermission::readWrite);
    valueIface->register_property("MaxValue", 127.0);
    valueIface->register_property("MinValue", -128.0);
    valueIface->register_property("Unit", 
        std::string("xyz.openbmc_project.Sensor.Value.Unit.DegreesC"));
    valueIface->initialize();
    
    auto warningIface = objectServer.add_interface(objectPath, warningInterface);
    warningIface->register_property("WarningHigh", warningHigh);
    warningIface->register_property("WarningAlarmHigh", warningAlarmHigh,
        sdbusplus::asio::PropertyPermission::readWrite);
    warningIface->initialize();
    
    auto criticalIface = objectServer.add_interface(objectPath, criticalInterface);
    criticalIface->register_property("CriticalHigh", criticalHigh);
    criticalIface->register_property("CriticalAlarmHigh", criticalAlarmHigh,
        sdbusplus::asio::PropertyPermission::readWrite);
    criticalIface->initialize();
    
    auto availIface = objectServer.add_interface(objectPath, availabilityInterface);
    availIface->register_property("Available", sensorAvailable,
        sdbusplus::asio::PropertyPermission::readWrite);  //動態
    availIface->initialize();
    
    auto operIface = objectServer.add_interface(objectPath, operationalInterface);
    operIface->register_property("Functional", true,
        sdbusplus::asio::PropertyPermission::readWrite);  //動態
    operIface->initialize();
    
    std::cout << "TEST_Temp sensor registered at " << objectPath << std::endl;
    std::cout << "Warning threshold: " << warningHigh << "C" << std::endl;
    std::cout << "Critical threshold: " << criticalHigh << "C" << std::endl;
    
    boost::asio::steady_timer timer(io);
    std::function<void(const boost::system::error_code&)> readTemp;
    
    readTemp = [&](const boost::system::error_code& ec) {
        if (ec) return;

        // ⭐ 新增：檢查檔案是否存在
        bool fileExists = std::filesystem::exists(tempFilePath);
        // ⭐ 新增：如果存在狀態改變，更新 D-Bus 屬性
        if (fileExists != sensorAvailable)
        {
            sensorAvailable = fileExists;
            availIface->set_property("Available", sensorAvailable);
            operIface->set_property("Functional", sensorAvailable);
            std::cout << "Sensor availability changed to: " 
                      << (sensorAvailable ? "Available" : "Unavailable") << std::endl;
        }
        // ⭐ 新增：根據 sensor 狀態決定要不要讀取
        if (sensorAvailable)
        {
            double temp = readTemperature();
            if (!std::isnan(temp))
            {
                valueIface->set_property("Value", temp);
                checkThresholds(temp, warningIface, criticalIface, conn);
            }
        }
        else
        {   
            // ⭐ 新增：檔案不存在時，Value 設為 NaN
            valueIface->set_property("Value", std::numeric_limits<double>::quiet_NaN());
        }
        
        timer.expires_after(std::chrono::seconds(1));
        timer.async_wait(readTemp);
    };
    
    timer.expires_after(std::chrono::seconds(1));
    timer.async_wait(readTemp);
    
    io.run();
    return 0;
}
