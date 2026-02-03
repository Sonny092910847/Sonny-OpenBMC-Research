#include "sensor_monitor.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

// Default configuration values
constexpr const char* defaultFilePath = "/tmp/sensor_value";
constexpr double defaultCriticalHigh = 85.0;
constexpr double defaultCriticalLow = 5.0;
constexpr int defaultPollIntervalMs = 1000;

// D-Bus service name
constexpr const char* serviceName = "xyz.openbmc_project.SensorMonitor";

/**
 * @brief Get environment variable or default value
 */
std::string getEnvOrDefault(const char* name, const char* defaultValue)
{
    const char* value = std::getenv(name);
    return (value != nullptr) ? value : defaultValue;
}

double getEnvOrDefault(const char* name, double defaultValue)
{
    const char* value = std::getenv(name);
    if (value != nullptr)
    {
        try
        {
            return std::stod(value);
        }
        catch (...)
        {
            return defaultValue;
        }
    }
    return defaultValue;
}

int getEnvOrDefault(const char* name, int defaultValue)
{
    const char* value = std::getenv(name);
    if (value != nullptr)
    {
        try
        {
            return std::stoi(value);
        }
        catch (...)
        {
            return defaultValue;
        }
    }
    return defaultValue;
}

int main()
{
    // 立即輸出到 stderr 確認程式啟動
    std::cerr << "[sensor-monitor] Program started" << std::endl;
    std::cerr << "[sensor-monitor] Initializing..." << std::endl;

    try
    {
        // Read configuration from environment variables
        std::string filePath = getEnvOrDefault("SENSOR_FILE", defaultFilePath);
        double criticalHigh =
            getEnvOrDefault("CRITICAL_HIGH", defaultCriticalHigh);
        double criticalLow =
            getEnvOrDefault("CRITICAL_LOW", defaultCriticalLow);
        int pollIntervalMs =
            getEnvOrDefault("POLL_INTERVAL_MS", defaultPollIntervalMs);

        std::cerr << "[sensor-monitor] Configuration:" << std::endl;
        std::cerr << "  File: " << filePath << std::endl;
        std::cerr << "  CriticalHigh: " << criticalHigh << std::endl;
        std::cerr << "  CriticalLow: " << criticalLow << std::endl;
        std::cerr << "  PollInterval: " << pollIntervalMs << "ms" << std::endl;

        // Create I/O context
        std::cerr << "[sensor-monitor] Creating I/O context..." << std::endl;
        boost::asio::io_context io;

        // Create D-Bus connection
        std::cerr << "[sensor-monitor] Connecting to D-Bus..." << std::endl;
        auto bus = std::make_shared<sdbusplus::asio::connection>(io);

        // Request the service name
        std::cerr << "[sensor-monitor] Requesting service name: " << serviceName
                  << std::endl;
        bus->request_name(serviceName);
        std::cerr << "[sensor-monitor] Service name acquired successfully"
                  << std::endl;

        // Create object server
        std::cerr << "[sensor-monitor] Creating object server..." << std::endl;
        sdbusplus::asio::object_server objServer(bus);

        // Create the sensor monitor
        std::cerr << "[sensor-monitor] Creating SensorMonitor object..."
                  << std::endl;
        sensor_monitor::SensorMonitor monitor(
            *bus, objServer, filePath, criticalHigh, criticalLow,
            std::chrono::milliseconds(pollIntervalMs));

        // Start monitoring
        std::cerr << "[sensor-monitor] Starting file monitoring..." << std::endl;
        monitor.startMonitoring();

        // Set up signal handling for graceful shutdown
        boost::asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait(
            [&io](const boost::system::error_code& /* ec */, int signo) {
                std::cerr << "[sensor-monitor] Received signal " << signo
                          << ", shutting down..." << std::endl;
                io.stop();
            });

        std::cerr << "[sensor-monitor] Service started successfully!"
                  << std::endl;
        std::cerr << "[sensor-monitor] Waiting for events..." << std::endl;

        // Run the I/O context
        io.run();

        std::cerr << "[sensor-monitor] Service stopped normally" << std::endl;
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr << "[sensor-monitor] D-Bus error: " << e.what() << std::endl;
        std::cerr << "[sensor-monitor] D-Bus name: " << e.name() << std::endl;
        std::cerr << "[sensor-monitor] D-Bus description: " << e.description()
                  << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[sensor-monitor] Exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "[sensor-monitor] Unknown exception occurred" << std::endl;
        return 1;
    }

    return 0;
}
