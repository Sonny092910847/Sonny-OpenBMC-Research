/**
 * GPIO Monitor Service for OpenBMC
 * 
 * This service monitors GPIOA1 (line 1) using polling mode
 * and outputs a message when the GPIO state changes.
 * 
 * Polling mode allows gpioset to change GPIO value between reads.
 */

#include <gpiod.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>

// GPIO Configuration
constexpr auto gpioChip = "gpiochip0";
constexpr auto gpioLine = 1;  // GPIOA1
constexpr auto gpioName = "GPIOA1";
constexpr auto pollIntervalMs = 1000;  // Poll every 1 second

int readGpioValue()
{
    // Open chip and get line within this scope
    // GPIO is released when function returns
    gpiod::chip chip(gpioChip);
    auto line = chip.get_line(gpioLine);
    
    gpiod::line_request config;
    config.consumer = "gpio-monitor";
    config.request_type = gpiod::line_request::DIRECTION_INPUT;
    
    line.request(config);
    int value = line.get_value();
    // line is automatically released when it goes out of scope
    
    return value;
}

int main()
{
    std::cout << "[GPIO Monitor] Starting GPIO monitor service (polling mode)..." 
              << std::endl;
    std::cout << "[GPIO Monitor] Monitoring: " << gpioName 
              << " (chip: " << gpioChip << ", line: " << gpioLine << ")" 
              << std::endl;
    std::cout << "[GPIO Monitor] Poll interval: " << pollIntervalMs << "ms" 
              << std::endl;

    try
    {
        // Get initial value
        int lastValue = readGpioValue();
        std::cout << "[GPIO Monitor] Initial value: " << lastValue << std::endl;
        std::cout << "[GPIO Monitor] Monitoring for state changes..." << std::endl;
        std::cout << "[GPIO Monitor] (Use 'gpioset gpiochip0 1=1' or '1=0' to test)"
                  << std::endl;

        int pollCount = 0;
        
        // Main polling loop
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
            
            int currentValue = readGpioValue();
            pollCount++;

            if (currentValue != lastValue)
            {
                std::string eventType;
                if (currentValue > lastValue)
                {
                    eventType = "RISING_EDGE (0 -> 1)";
                }
                else
                {
                    eventType = "FALLING_EDGE (1 -> 0)";
                }

                std::cout << "[GPIO Monitor] *** State Change Detected! ***" << std::endl;
                std::cout << "[GPIO Monitor]   Type: " << eventType << std::endl;
                std::cout << "[GPIO Monitor]   Previous value: " << lastValue << std::endl;
                std::cout << "[GPIO Monitor]   Current value: " << currentValue << std::endl;

                lastValue = currentValue;
            }
            else
            {
                // Every 5 polls, show status
                if (pollCount % 5 == 0)
                {
                    std::cout << "[GPIO Monitor] Polling... (current value: " 
                              << currentValue << ")" << std::endl;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[GPIO Monitor] Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
