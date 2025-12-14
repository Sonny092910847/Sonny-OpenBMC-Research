#include <ipmid/api.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

constexpr ipmi::NetFn netFnOem = ipmi::netFnOemOne;  // 0x30
constexpr ipmi::Cmd cmdSetTestTemp = 0x01;
constexpr ipmi::Cmd cmdSetTmp75Override = 0x02;  // 新增：TMP75 Override

constexpr auto tempFilePath = "/tmp/test/test_temp";


ipmi::RspType<> ipmiSetTestTemp(ipmi::Context::ptr, uint8_t tempValue)
{
    std::filesystem::create_directories("/tmp/test");
    
    std::ofstream file(tempFilePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open " << tempFilePath << std::endl;
        return ipmi::responseUnspecifiedError();
    }
    
    file << static_cast<double>(tempValue);
    file.close();
    
    std::cout << "OEM Command: Set TEST_Temp to " << static_cast<int>(tempValue) << "°C" << std::endl;
    
    return ipmi::responseSuccess();
}

// ===== Cmd 0x02: Set TMP75 Override =====
// Usage: ipmitool raw 0x30 0x02 <temp_value>
// Example: ipmitool raw 0x30 0x02 0x4B (set to 75°C)
//          ipmitool raw 0x30 0x02 0x00 (restore real value)

std::string findTmp75OverridePath()  //自動搜尋 TMP75 的 sysfs 路徑
{
    // hwmon 裝置的基礎路徑
    const std::string hwmonBase = "/sys/class/hwmon/";
    
    // 遍歷 /sys/class/hwmon/ 下的所有目錄 (hwmon0, hwmon1, hwmon2...)
    for (const auto& entry : std::filesystem::directory_iterator(hwmonBase))
    {
        // 組合出 name 檔案的路徑，例如 /sys/class/hwmon/hwmon2/name
        std::string namePath = entry.path().string() + "/name";

        // 嘗試開啟 name 檔案
        std::ifstream nameFile(namePath);
        if (nameFile.is_open())
        {
            // 讀取裝置名稱
            std::string name;
            std::getline(nameFile, name);     // EX: 讀到 "tmp75" 或 "lm75"
            nameFile.close();
            
            if (name.find("tmp75") != std::string::npos || 
                name.find("lm75") != std::string::npos)
            {
                // 組合 override_tmp75 的完整路徑
                std::string overridePath = entry.path().string() + "/override_tmp75";
                if (std::filesystem::exists(overridePath))
                {
                    return overridePath;
                }
            }
        }
    }
    return "";  // 沒找到，回傳空字串
}

ipmi::RspType<> ipmiSetTmp75Override(ipmi::Context::ptr, uint8_t tempValue)
{
    //取得 override_tmp75 的路徑
    std::string overridePath = findTmp75OverridePath();
    
    if (overridePath.empty())
    {
        std::cerr << "TMP75 override_tmp75 not found" << std::endl;
        return ipmi::responseDestinationUnavailable();
    }
    
    //開啟 sysfs 檔案
    std::ofstream file(overridePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open " << overridePath << std::endl;
        return ipmi::responseUnspecifiedError();
    }

    //寫入溫度值 (ex: 從Host發送IPMI Command 0x50 = 80   ->   寫入到/sys/class/hwmon/hwmon2/override_tmp75)
    file << static_cast<int>(tempValue);
    file.close();
    
    std::cout << "OEM Command: Set TMP75 override to " << static_cast<int>(tempValue) 
              << "°C (path: " << overridePath << ")" << std::endl;
    
    return ipmi::responseSuccess();
}

//註冊
void registerOEMFunctions() __attribute__((constructor));
void registerOEMFunctions()
{
    std::cout << "Registering TEST_Temp OEM commands..." << std::endl;
    
    // Cmd 0x01
    ipmi::registerHandler(
        ipmi::prioOemBase,
        netFnOem,
        cmdSetTestTemp,
        ipmi::Privilege::Admin,
        ipmiSetTestTemp
    );
    
    // Cmd 0x02
    ipmi::registerHandler(
        ipmi::prioOemBase,
        netFnOem,
        cmdSetTmp75Override,
        ipmi::Privilege::Admin,
        ipmiSetTmp75Override
    );
    
    std::cout << "OEM commands registered: NetFn=0x30, Cmd=0x01 (TEST_Temp), Cmd=0x02 (TMP75)" << std::endl;
}
