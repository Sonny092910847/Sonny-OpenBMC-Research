/**
 * L2 Training - D-Bus Service with File Monitor
 * ID: 835051, Name: Sonny Chu
 *
 * Level 1: D-Bus Service with objectpath, interface, properties
 * Level 2: File monitoring - read value and update D-Bus property
 * Level 3: Threshold alert - send Signal when value crosses threshold
 */

#include <systemd/sd-bus.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/message.hpp>
#include <phosphor-logging/lg2.hpp>

#include <fstream>
#include <chrono>
#include <string>
#include <filesystem>

PHOSPHOR_LOG2_USING;

// D-Bus naming
constexpr auto SERVICE_NAME = "xyz.openbmc_project.Training.L2";
constexpr auto OBJECT_PATH = "/xyz/openbmc_project/training/l2";
constexpr auto INTERFACE_NAME = "xyz.openbmc_project.Training.L2.Monitor";

// Configuration
constexpr auto MONITOR_FILE = "/tmp/l2_monitor";
constexpr int32_t DEFAULT_THRESHOLD = 50;
constexpr auto POLL_INTERVAL_SEC = 2;

// Global state for D-Bus callbacks
static int32_t g_value = 0;
static int32_t g_threshold = DEFAULT_THRESHOLD;
static bool g_alertActive = false;
static sd_bus* g_bus = nullptr;

// Property getter callbacks
static int getValueProperty(sd_bus* /*bus*/, const char* /*path*/,
                            const char* /*interface*/, const char* /*property*/,
                            sd_bus_message* reply, void* /*userdata*/,
                            sd_bus_error* /*error*/)
{
    return sd_bus_message_append(reply, "i", g_value);
}

static int getThresholdProperty(sd_bus* /*bus*/, const char* /*path*/,
                                const char* /*interface*/,
                                const char* /*property*/,
                                sd_bus_message* reply, void* /*userdata*/,
                                sd_bus_error* /*error*/)
{
    return sd_bus_message_append(reply, "i", g_threshold);
}

static int getAlertActiveProperty(sd_bus* /*bus*/, const char* /*path*/,
                                  const char* /*interface*/,
                                  const char* /*property*/,
                                  sd_bus_message* reply, void* /*userdata*/,
                                  sd_bus_error* /*error*/)
{
    return sd_bus_message_append(reply, "b", g_alertActive);
}

// D-Bus vtable definition
static const sd_bus_vtable l2Vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_PROPERTY("Value", "i", getValueProperty, 0,
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("Threshold", "i", getThresholdProperty, 0,
                    SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("AlertActive", "b", getAlertActiveProperty, 0,
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_SIGNAL("ThresholdAlert", "bi", 0),
    SD_BUS_VTABLE_END};

// Emit PropertiesChanged signal
static void emitPropertyChanged(const char* propName, int32_t intVal,
                                bool boolVal, bool isInt)
{
    sd_bus_message* msg = nullptr;
    int r;

    r = sd_bus_message_new_signal(g_bus, &msg, OBJECT_PATH,
                                  "org.freedesktop.DBus.Properties",
                                  "PropertiesChanged");
    if (r < 0)
        return;

    r = sd_bus_message_append(msg, "s", INTERFACE_NAME);
    if (r < 0)
    {
        sd_bus_message_unref(msg);
        return;
    }

    r = sd_bus_message_open_container(msg, 'a', "{sv}");
    if (r < 0)
    {
        sd_bus_message_unref(msg);
        return;
    }

    r = sd_bus_message_open_container(msg, 'e', "sv");
    if (r < 0)
    {
        sd_bus_message_unref(msg);
        return;
    }

    r = sd_bus_message_append(msg, "s", propName);
    if (r < 0)
    {
        sd_bus_message_unref(msg);
        return;
    }

    if (isInt)
    {
        r = sd_bus_message_open_container(msg, 'v', "i");
        if (r < 0)
        {
            sd_bus_message_unref(msg);
            return;
        }
        r = sd_bus_message_append(msg, "i", intVal);
    }
    else
    {
        r = sd_bus_message_open_container(msg, 'v', "b");
        if (r < 0)
        {
            sd_bus_message_unref(msg);
            return;
        }
        r = sd_bus_message_append(msg, "b", boolVal);
    }
    if (r < 0)
    {
        sd_bus_message_unref(msg);
        return;
    }

    sd_bus_message_close_container(msg); // variant
    sd_bus_message_close_container(msg); // dict entry
    sd_bus_message_close_container(msg); // array

    // Empty array for invalidated properties
    sd_bus_message_append(msg, "as", 0);

    sd_bus_send(g_bus, msg, nullptr);
    sd_bus_message_unref(msg);
}

// Emit ThresholdAlert signal
static void emitThresholdAlert(bool triggered, int32_t val)
{
    sd_bus_message* msg = nullptr;

    int r = sd_bus_message_new_signal(g_bus, &msg, OBJECT_PATH, INTERFACE_NAME,
                                      "ThresholdAlert");
    if (r < 0)
        return;

    r = sd_bus_message_append(msg, "bi", triggered, val);
    if (r < 0)
    {
        sd_bus_message_unref(msg);
        return;
    }

    sd_bus_send(g_bus, msg, nullptr);
    sd_bus_message_unref(msg);

    info("ThresholdAlert signal sent: triggered={TRIGGERED}, value={VALUE}",
         "TRIGGERED", triggered, "VALUE", val);
}

// Check file and update value
static void checkFileAndUpdate()
{
    if (!std::filesystem::exists(MONITOR_FILE))
    {
        return;
    }

    std::ifstream file(MONITOR_FILE);
    if (!file.is_open())
    {
        return;
    }

    int32_t newValue = 0;
    if (!(file >> newValue))
    {
        return;
    }

    if (newValue != g_value)
    {
        g_value = newValue;
        info("Value updated: {VALUE}", "VALUE", g_value);

        // Emit PropertiesChanged for Value
        emitPropertyChanged("Value", g_value, false, true);

        // Check threshold crossing
        bool shouldAlert = (g_value > g_threshold);
        if (shouldAlert != g_alertActive)
        {
            g_alertActive = shouldAlert;

            // Emit PropertiesChanged for AlertActive
            emitPropertyChanged("AlertActive", 0, g_alertActive, false);

            // Emit ThresholdAlert signal
            emitThresholdAlert(g_alertActive, g_value);

            if (g_alertActive)
            {
                warning(
                    "ALERT: Value {VALUE} exceeded threshold {THRESHOLD}",
                    "VALUE", g_value, "THRESHOLD", g_threshold);
            }
            else
            {
                info(
                    "ALERT CLEARED: Value {VALUE} returned below threshold {THRESHOLD}",
                    "VALUE", g_value, "THRESHOLD", g_threshold);
            }
        }
    }
}

int main()
{
    info("L2 Training Application - ID: 835051, Name: Sonny Chu");

    int r;
    sd_bus_slot* slot = nullptr;

    // Connect to system bus
    r = sd_bus_open_system(&g_bus);
    if (r < 0)
    {
        error("Failed to connect to system bus: {ERROR}", "ERROR",
              strerror(-r));
        return 1;
    }

    // Add object vtable
    r = sd_bus_add_object_vtable(g_bus, &slot, OBJECT_PATH, INTERFACE_NAME,
                                 l2Vtable, nullptr);
    if (r < 0)
    {
        error("Failed to add object vtable: {ERROR}", "ERROR", strerror(-r));
        sd_bus_unref(g_bus);
        return 1;
    }

    // Request service name
    r = sd_bus_request_name(g_bus, SERVICE_NAME, 0);
    if (r < 0)
    {
        error("Failed to acquire service name: {ERROR}", "ERROR",
              strerror(-r));
        sd_bus_slot_unref(slot);
        sd_bus_unref(g_bus);
        return 1;
    }

    info("L2 Monitor started - Service: {SERVICE}, Path: {PATH}", "SERVICE",
         SERVICE_NAME, "PATH", OBJECT_PATH);
    info("Monitoring file: {FILE}, Threshold: {THRESHOLD}", "FILE",
         MONITOR_FILE, "THRESHOLD", g_threshold);

    // Main loop
    while (true)
    {
        // Check file and update value
        checkFileAndUpdate();

        // Process pending D-Bus events
        r = sd_bus_process(g_bus, nullptr);
        if (r < 0)
        {
            error("Failed to process bus: {ERROR}", "ERROR", strerror(-r));
            break;
        }

        // Wait for events with timeout
        r = sd_bus_wait(g_bus, POLL_INTERVAL_SEC * 1000000ULL);
        if (r < 0 && r != -EINTR)
        {
            error("Failed to wait on bus: {ERROR}", "ERROR", strerror(-r));
            break;
        }
    }

    sd_bus_slot_unref(slot);
    sd_bus_unref(g_bus);

    return 0;
}
