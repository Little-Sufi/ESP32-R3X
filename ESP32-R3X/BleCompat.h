#pragma once

#include <NimBLEDevice.h>

using BLEDevice                 = NimBLEDevice;
using BLEServer                 = NimBLEServer;
using BLEAdvertising            = NimBLEAdvertising;
using BLEAdvertisementData      = NimBLEAdvertisementData;
using BLEScan                   = NimBLEScan;
using BLEScanResults            = NimBLEScanResults;
using BLEAdvertisedDevice       = NimBLEAdvertisedDevice;
using BLEAdvertisedDeviceCallbacks = NimBLEAdvertisedDeviceCallbacks;
using BLEAddress                = NimBLEAddress;
using BLEUUID                   = NimBLEUUID;

#if !defined(SOC_BT_CLASSIC_SUPPORTED) || !SOC_BT_CLASSIC_SUPPORTED
typedef int esp_bt_gap_cb_event_t;
typedef struct {
    struct {
        uint8_t bda[6];
    } disc_res;
} esp_bt_gap_cb_param_t;
#ifndef ESP_BT_GAP_DISC_RES_EVT
#define ESP_BT_GAP_DISC_RES_EVT 0
#endif
#endif
