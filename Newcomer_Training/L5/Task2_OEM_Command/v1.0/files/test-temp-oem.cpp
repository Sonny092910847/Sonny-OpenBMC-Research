#include <ipmid/api.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

 
constexpr ipmi::NetFn netFnOem = ipmi::netFnOemOne;  // Network Function，0x30 是 OEM 保留區段(廠商自定義命令區段)
constexpr ipmi::Cmd cmdSetTestTemp = 0x01; //Cmd 0x01:我自定義的命令編號

// 使用與Task1相同的資料夾
constexpr auto tempFilePath = "/tmp/test/test_temp";

//設定溫度
ipmi::RspType<> ipmiSetTestTemp(ipmi::Context::ptr, uint8_t tempValue) 
{
    // Step 1: 建立目錄 
    std::filesystem::create_directories("/tmp/test");
    
    // Step 2: 開啟檔案 
    std::ofstream file(tempFilePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open " << tempFilePath << std::endl;
        return ipmi::responseUnspecifiedError();
    }
    
    // Step 3: 寫入溫度 
    file << static_cast<double>(tempValue); // uint8_t -> double
    file.close(); // Step 4: 關閉檔案  
    
    std::cout << "OEM Command: Set TEST_Temp to " << static_cast<int>(tempValue) << "°C" << std::endl; // Step 5: 印出 log 
    
    return ipmi::responseSuccess(); // Step 6: 回傳成功
}

void registerOEMFunctions() __attribute__((constructor)); //__attribute__((constructor))讓這個函式向 ipmid 註冊命令
void registerOEMFunctions()
{
    std::cout << "Registering TEST_Temp OEM commands..." << std::endl;
    
    ipmi::registerHandler(
        ipmi::prioOemBase,       // 優先順序（OEM 基礎優先權） 
        netFnOem,                // NetFn = 0x30
        cmdSetTestTemp,          // Cmd = 0x01
        ipmi::Privilege::Admin,  // 需要 Admin 權限才能執行
        ipmiSetTestTemp          // 實際處理函式的指標
    );
    
    std::cout << "TEST_Temp OEM command registered: NetFn=0x30, Cmd=0x01" << std::endl;
}
