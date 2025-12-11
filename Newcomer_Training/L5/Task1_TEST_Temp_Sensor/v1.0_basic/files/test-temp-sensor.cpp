#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

/*D-Bus配置: 給字串取個變數名(=後面)，方便後面使用。
此時D-Bus上甚麼都還沒有*/
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
        // 1. 建立 D-Bus Method Call
        auto method = conn.new_method_call(selLoggerService, selLoggerPath,
                                           selLoggerInterface, "IpmiSelAdd");
        // 2. 把參數附加到 Method Call 上
        method.append(message, path, eventData, assert, genId);
        // 3. 發送呼叫（不等待回應）
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
    file >> temp; //把溫度寫入檔案
    return temp;
}

//門檻值檢查函式
void checkThresholds(double value,
                     std::shared_ptr<sdbusplus::asio::dbus_interface> warningIface,
                     std::shared_ptr<sdbusplus::asio::dbus_interface> criticalIface,
                     std::shared_ptr<sdbusplus::asio::connection> conn)
{
    //檢查Warning 門檻（80°C）
    bool newWarningAlarm = (value >= warningHigh);
    if (newWarningAlarm != warningAlarmHigh) //若狀態改變 -> 更新D-Bus上的屬性
    {
        warningAlarmHigh = newWarningAlarm;
        warningIface->set_property("WarningAlarmHigh", warningAlarmHigh); //set_property更新DBus屬性
        std::cout << "Warning alarm changed to: " << (warningAlarmHigh ? "true" : "false") 
                  << " (value=" << value << ")" << std::endl;
      
        // 寫入 SEL 記錄
        std::vector<uint8_t> eventData = {0x01, 0x07, static_cast<uint8_t>(value)};
        addSelEntry(*conn, "TEST_Temp Warning Threshold", objectPath, eventData, warningAlarmHigh, 0x2000);
    }

    //檢查 Critical 門檻（90°C)
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
    //檔案初始化
    std::filesystem::create_directories("/tmp/test"); // v1.0: 確保目錄存在
    
    if (!std::filesystem::exists(tempFilePath)) // v1.0: 如果檔案不存在，自動建立並寫入初始值
    {
        std::ofstream initFile(tempFilePath);
        initFile << "25.0";
        initFile.close();
        std::cout << "Created initial temperature file with value 25.0" << std::endl;
    }
    //D-Bus連線
    boost::asio::io_context io; 
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);   
    conn->request_name(serviceName);                               //注意: Service Name只能被一個程式占用
    sdbusplus::asio::object_server objectServer(conn);


    //在同一個Object Path上註冊5個Interface
    auto valueIface = objectServer.add_interface(objectPath, valueInterface);                                                                              
    double currentValue = readTemperature();
    valueIface->register_property("Value", currentValue,                        
        sdbusplus::asio::PropertyPermission::readWrite);
    valueIface->register_property("MaxValue", 127.0);                           //註冊其他資訊(threshold)的Property
    valueIface->register_property("MinValue", -128.0);
    valueIface->register_property("Unit", 
        std::string("xyz.openbmc_project.Sensor.Value.Unit.DegreesC"));         
    valueIface->initialize(); //服務上線

    
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

    //循環定時器: 讓sensor每秒更新一次
    boost::asio::steady_timer timer(io);
    std::function<void(const boost::system::error_code&)> readTemp;
    
    readTemp = [&](const boost::system::error_code& ec) {
    //         ^^^
    //         捕獲子句：[&] 表示「捕獲外部所有變數的參考」
    //
    //         這樣在 lambda 裡面可以直接使用：
    //         - timer
    //         - valueIface
    //         - warningIface
    //         - criticalIface
    //         - conn
    //         - readTemp 自己（遞迴用）
        if (ec) return;

        //每秒執行的工作:
        double temp = readTemperature();                              // 1. 呼叫readTemperature讀取溫度檔案
        if (!std::isnan(temp))                                        // 2. 如果讀取到有效數值
        {
            valueIface->set_property("Value", temp);                  // 3. 更新D-Bus
            checkThresholds(temp, warningIface, criticalIface, conn); // 4. 呼叫checkThresholds檢查是否超過門檻
        }
        
        timer.expires_after(std::chrono::seconds(1));                 // 5. 1 秒後再執行
        timer.async_wait(readTemp);                                   // 6. 等待並呼叫自己
    };
    
    timer.expires_after(std::chrono::seconds(1));
    timer.async_wait(readTemp);
    
    io.run();                                                          // 進入事件循環（程式會一直跑，不會結束）             
    return 0;
}
