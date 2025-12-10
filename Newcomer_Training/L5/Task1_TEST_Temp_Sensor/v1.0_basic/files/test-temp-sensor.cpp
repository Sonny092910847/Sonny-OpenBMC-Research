#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

//D-Bus配置
static constexpr auto serviceName = "xyz.openbmc_project.TEST_Temp";
static constexpr auto objectPath = "/xyz/openbmc_project/sensors/temperature/TEST_Temp";
static constexpr auto valueInterface = "xyz.openbmc_project.Sensor.Value";
static constexpr auto warningInterface = "xyz.openbmc_project.Sensor.Threshold.Warning";
static constexpr auto criticalInterface = "xyz.openbmc_project.Sensor.Threshold.Critical";
static constexpr auto availabilityInterface = "xyz.openbmc_project.State.Decorator.Availability";
static constexpr auto operationalInterface = "xyz.openbmc_project.State.Decorator.OperationalStatus";

//全域變數
static constexpr auto tempFilePath = "/tmp/test/test_temp";
static constexpr double warningHigh = 80.0;
static constexpr double criticalHigh = 90.0;

bool warningAlarmHigh = false;
bool criticalAlarmHigh = false;

static constexpr auto selLoggerService = "xyz.openbmc_project.Logging.IPMI";
static constexpr auto selLoggerPath = "/xyz/openbmc_project/Logging/IPMI";
static constexpr auto selLoggerInterface = "xyz.openbmc_project.Logging.IPMI";

//SEL日誌函式
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

//溫度讀取函式
double readTemperature()
{
    std::ifstream file(tempFilePath);
    if (!file.is_open())
    {
        std::cerr << "Cannot open temperature file: " << tempFilePath << std::endl;
        return std::numeric_limits<double>::quiet_NaN();
    }
    double temp;
    file >> temp;
    return temp;
}

//門檻值檢查函式
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

// main v1.0: 自動建立檔案
int main()
{
    std::cout << "TEST_Temp Sensor Service starting..." << std::endl;
    
    std::filesystem::create_directories("/tmp/test"); // v1.0: 確保目錄存在
    
    if (!std::filesystem::exists(tempFilePath)) // v1.0: 如果檔案不存在，自動建立並寫入初始值
    {
        std::ofstream initFile(tempFilePath);
        initFile << "25.0";
        initFile.close();
        std::cout << "Created initial temperature file with value 25.0" << std::endl;
    }

    boost::asio::io_context io; //排程(事件循環)
    auto conn = std::make_shared<sdbusplus::asio::connection>(io); /*建立D-Bus連線  
                                                                     std::make_shared<>: 建立一個「共享指標」，多個地方可以共用這個連線
                                                                     sdbusplus::asio::connection: D-Bus連線物件，用來與D-Bus daemon溝通
                                                                     (io): 把事件循環傳進去，讓 D-Bus 訊息由 io_context 管理
                                                                     auto conn: 自動推斷型別*/
  
    // serviceName = "xyz.openbmc_project.TEST_Temp"               //在D-Bus上註冊Service Name -> 之後就可以使用busctl透過這個名字看到此服務     
    conn->request_name(serviceName);                               //注意: Service Name只能被一個程式占用
  
    //建立Object Server(準備提供服務)                                //conn: 綁定到剛才建立的D-Bus連線
    sdbusplus::asio::object_server objectServer(conn);
  
    //在指定路徑上建立 Interface 並註冊 Properties
    auto valueIface = objectServer.add_interface(objectPath, valueInterface);  /*objectPath = "/xyz/openbmc_project/sensors/temperature/TEST_Temp"
                                                                                 valueInterface = "xyz.openbmc_project.Sensor.Value"
                                                                                 在這個地址：/xyz/openbmc_project/sensors/temperature/TEST_Temp 
                                                                                 建立一個叫做這個名字的 Interface：xyz.openbmc_project.Sensor.Value
    //讀取初始溫度                                                                             但還沒有任何 Property！ 要用 register_property 來新增*/
    double currentValue = readTemperature();
    valueIface->register_property("Value", currentValue,                        //Value: Property的名稱, currentValuw: Property初始值, readWrite: 權限: 可讀寫
        sdbusplus::asio::PropertyPermission::readWrite);
    valueIface->register_property("MaxValue", 127.0);                           //註冊其他資訊(threshold)的Property
    valueIface->register_property("MinValue", -128.0);
    valueIface->register_property("Unit", 
        std::string("xyz.openbmc_project.Sensor.Value.Unit.DegreesC"));         //註冊 Unit Property: important!!
    valueIface->initialize(); //服務上線

    //在同一個Object Path上註冊多個Interface
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
    availIface->register_property("Available", true); // v1.0: 固定為true
    availIface->initialize();
    
    auto operIface = objectServer.add_interface(objectPath, operationalInterface);
    operIface->register_property("Functional", true); // v1.0: 固定為true
    operIface->initialize();
    
    std::cout << "TEST_Temp sensor registered at " << objectPath << std::endl;
    std::cout << "Warning threshold: " << warningHigh << "C" << std::endl;
    std::cout << "Critical threshold: " << criticalHigh << "C" << std::endl;
    
    boost::asio::steady_timer timer(io);
    std::function<void(const boost::system::error_code&)> readTemp;
    
    readTemp = [&](const boost::system::error_code& ec) {
        if (ec) return;

        // v1.0: 直接讀取，不檢查檔案存在，如果讀不到就跳過這一輪。
        double temp = readTemperature();
        if (!std::isnan(temp))
        {
            valueIface->set_property("Value", temp);
            checkThresholds(temp, warningIface, criticalIface, conn);
        }
        
        timer.expires_after(std::chrono::seconds(1));
        timer.async_wait(readTemp);
    };
    
    timer.expires_after(std::chrono::seconds(1));
    timer.async_wait(readTemp);
    
    io.run();
    return 0;
}
