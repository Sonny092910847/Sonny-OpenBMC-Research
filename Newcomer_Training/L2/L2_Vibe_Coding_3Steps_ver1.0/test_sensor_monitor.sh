#!/bin/bash
# Test script for sensor-monitor service
# This script demonstrates the functionality of all three levels

set -e

SENSOR_FILE="/tmp/sensor_value"
SERVICE_NAME="xyz.openbmc_project.SensorMonitor"
OBJECT_PATH="/xyz/openbmc_project/sensors/temperature/monitored_sensor"
VALUE_INTERFACE="xyz.openbmc_project.Sensor.Value"
THRESHOLD_INTERFACE="xyz.openbmc_project.Sensor.Threshold.Critical"

echo "=== Sensor Monitor Test Script ==="
echo ""

# Create test sensor file
echo "Creating test sensor file: $SENSOR_FILE"
echo "25.0" > "$SENSOR_FILE"

# Wait for service to read the value
sleep 2

echo ""
echo "=== Level 1: D-Bus Interface Test ==="
echo "Reading D-Bus properties..."
echo ""

# Read Value property
echo "Value:"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$VALUE_INTERFACE" Value || echo "  (service not running)"

# Read Unit property
echo "Unit:"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$VALUE_INTERFACE" Unit || echo "  (service not running)"

# Read MaxValue property
echo "MaxValue:"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$VALUE_INTERFACE" MaxValue || echo "  (service not running)"

# Read MinValue property
echo "MinValue:"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$VALUE_INTERFACE" MinValue || echo "  (service not running)"

echo ""
echo "=== Level 2: File Monitoring Test ==="
echo "Updating sensor file to 50.0..."
echo "50.0" > "$SENSOR_FILE"
sleep 2

echo "Reading updated value:"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$VALUE_INTERFACE" Value || echo "  (service not running)"

echo ""
echo "=== Level 3: Threshold Alarm Test ==="

echo ""
echo "Reading threshold properties:"
echo "CriticalHigh:"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$THRESHOLD_INTERFACE" CriticalHigh || echo "  (service not running)"

echo "CriticalLow:"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$THRESHOLD_INTERFACE" CriticalLow || echo "  (service not running)"

echo ""
echo "Setting value above critical high (90.0)..."
echo "90.0" > "$SENSOR_FILE"
sleep 2

echo "CriticalAlarmHigh (should be true):"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$THRESHOLD_INTERFACE" CriticalAlarmHigh || echo "  (service not running)"

echo ""
echo "Setting value below critical low (3.0)..."
echo "3.0" > "$SENSOR_FILE"
sleep 2

echo "CriticalAlarmLow (should be true):"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$THRESHOLD_INTERFACE" CriticalAlarmLow || echo "  (service not running)"

echo ""
echo "Setting value to normal range (40.0)..."
echo "40.0" > "$SENSOR_FILE"
sleep 2

echo "CriticalAlarmHigh (should be false):"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$THRESHOLD_INTERFACE" CriticalAlarmHigh || echo "  (service not running)"

echo "CriticalAlarmLow (should be false):"
busctl get-property "$SERVICE_NAME" "$OBJECT_PATH" "$THRESHOLD_INTERFACE" CriticalAlarmLow || echo "  (service not running)"

echo ""
echo "=== Test Complete ==="
echo ""
echo "To monitor D-Bus signals, run in another terminal:"
echo "  busctl monitor $SERVICE_NAME"
