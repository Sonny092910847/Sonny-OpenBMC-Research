#!/bin/bash

# Script to clone core OpenBMC repositories (excluding vendor-specific ones)
# Destination: OpenBMC v2.18 directory

cd "/home/user/Sonny-OpenBMC-Research/OpenBMC v2.18"

# Core OpenBMC repositories (non-vendor specific)
repos=(
    # Web and API
    "bmcweb"

    # D-Bus libraries and interfaces
    "sdbusplus"
    "phosphor-dbus-interfaces"
    "phosphor-objmgr"

    # Logging and debugging
    "phosphor-logging"
    "phosphor-debug-collector"
    "phosphor-hostlogger"

    # IPMI
    "phosphor-host-ipmid"
    "phosphor-net-ipmid"
    "ipmitool"

    # State and lifecycle management
    "phosphor-state-manager"
    "phosphor-watchdog"
    "phosphor-post-code-manager"

    # Code and firmware management
    "phosphor-bmc-code-mgmt"
    "phosphor-software-manager"

    # Hardware monitoring and sensors
    "phosphor-hwmon"
    "phosphor-fan-presence"
    "phosphor-nvme"
    "dbus-sensors"
    "entity-manager"

    # User and security management
    "phosphor-user-manager"
    "phosphor-certificate-manager"

    # Network management
    "phosphor-networkd"

    # Power and thermal
    "phosphor-power"

    # LED management
    "phosphor-led-manager"
    "phosphor-led-sysfs"

    # Time management
    "phosphor-time-manager"

    # Inventory
    "phosphor-inventory-manager"

    # Settings
    "phosphor-settings-manager"

    # PID control
    "phosphor-pid-control"

    # Host control
    "phosphor-host-postd"

    # Utilities
    "phosphor-mboxd"
    "phosphor-ipmi-blobs"
    "phosphor-ipmi-flash"

    # Documentation and tools
    "docs"
    "openbmc-test-automation"
    "openbmc-tools"

    # Build system components
    "meta-phosphor"
    "meta-openembedded"

    # Virtual media
    "jsnbd"

    # Additional core services
    "phosphor-snmp"
    "phosphor-virtual-sensor"
    "phosphor-health-monitor"
    "phosphor-sel-logger"
)

echo "Starting to clone ${#repos[@]} core OpenBMC repositories..."
echo "This may take a while..."
echo ""

success_count=0
failed_count=0
failed_repos=()

for repo in "${repos[@]}"; do
    echo "Cloning $repo..."
    if git clone --depth 1 "https://github.com/openbmc/${repo}.git" 2>&1 | grep -v "^Cloning"; then
        ((success_count++))
        echo "✓ $repo cloned successfully"
    else
        ((failed_count++))
        failed_repos+=("$repo")
        echo "✗ $repo failed to clone"
    fi
    echo ""
done

echo "========================================"
echo "Clone Summary:"
echo "Total: ${#repos[@]} repositories"
echo "Success: $success_count"
echo "Failed: $failed_count"

if [ $failed_count -gt 0 ]; then
    echo ""
    echo "Failed repositories:"
    for repo in "${failed_repos[@]}"; do
        echo "  - $repo"
    done
fi
echo "========================================"
