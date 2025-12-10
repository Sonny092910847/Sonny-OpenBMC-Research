#include <ipmid/api.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

// OEM NetFn and Command definitions
constexpr ipmi::NetFn netFnOem = ipmi::netFnOemOne;  // 0x30
constexpr ipmi::Cmd cmdSetTestTemp = 0x01;

// Temperature file path (same as Task 1)
constexpr auto tempFilePath = "/tmp/test/test_temp";

// OEM Command: Set TEST_Temp value
// Usage: ipmitool raw 0x30 0x01 <temp_value>
// Example: ipmitool raw 0x30 0x01 0x55 (set to 85°C)
ipmi::RspType<> ipmiSetTestTemp(ipmi::Context::ptr, uint8_t tempValue)
{
    // Create directory if not exists
    std::filesystem::create_directories("/tmp/test");
    
    // Write temperature to file
    std::ofstream file(tempFilePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open " << tempFilePath << std::endl;
        return ipmi::responseUnspecifiedError();
    }
    
    // Write temperature as double
    file << static_cast<double>(tempValue);
    file.close();
    
    std::cout << "OEM Command: Set TEST_Temp to " << static_cast<int>(tempValue) << "°C" << std::endl;
    
    return ipmi::responseSuccess();
}

void registerOEMFunctions() __attribute__((constructor));
void registerOEMFunctions()
{
    std::cout << "Registering TEST_Temp OEM commands..." << std::endl;
    
    ipmi::registerHandler(
        ipmi::prioOemBase,
        netFnOem,
        cmdSetTestTemp,
        ipmi::Privilege::Admin,
        ipmiSetTestTemp
    );
    
    std::cout << "TEST_Temp OEM command registered: NetFn=0x30, Cmd=0x01" << std::endl;
}
