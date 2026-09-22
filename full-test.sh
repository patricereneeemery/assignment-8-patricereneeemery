#!/bin/bash
set -e

# Cleanup from previous runs
sudo rmmod hello 2>/dev/null || true
sudo rmmod faulty 2>/dev/null || true
sudo rmmod scull 2>/dev/null || true

echo "=== Assignment 7 Test Start ==="

echo "[1] Building modules..."
make

echo "[2] Loading hello.ko..."
sudo insmod module/misc-modules/hello.ko || { echo "FAILED: hello.ko load"; exit 1; }

echo "[3] Checking dmesg for hello..."
sudo dmesg | tail -n 20 | grep -i "hello" || { echo "FAILED: hello message"; exit 1; }

echo "[4] Unloading hello.ko..."
sudo rmmod hello || { echo "FAILED: hello.ko unload"; exit 1; }

echo "[5] Loading faulty.ko (expected crash)..."
sudo insmod module/misc-modules/faulty.ko || echo "[EXPECTED] faulty.ko intentionally fails"

echo "[6] Loading scull.ko..."
sudo insmod module/scull/scull.ko || { echo "FAILED: scull.ko load"; exit 1; }

echo "[6.1] Detecting SCULL major numbers..."
grep scull /proc/devices

MAJOR=$(grep scull /proc/devices | head -n1 | awk '{print $1}')

echo "[7] Creating scull device node..."
sudo mknod /dev/scull c $MAJOR 0 || true

echo "[8] Testing scull read/write..."
echo "test" | sudo tee /dev/scull > /dev/null
sudo cat /dev/scull || { echo "FAILED: scull read"; exit 1; }

echo "[9] Unloading scull.ko..."
sudo rmmod scull || { echo "FAILED: scull.ko unload"; exit 1; }


echo "=== Assignment 7 Test Complete: SUCCESS ==="
exit 0
