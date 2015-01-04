/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "BLEDevice.h"

#define SHORT_NAME              ("5280DOOR")    // Keep this short: max 8 chars if a 128bit UUID is also advertised.

/**
 * For this demo application, populate the beacon advertisement payload
 * with 2 AD structures: FLAG and MSD (manufacturer specific data).
 *
 * Reference:
 *  Bluetooth Core Specification 4.0 (Vol. 3), Part C, Section 11, 18
 */

BLEDevice ble;

    /**
     * The Beacon payload (encapsulated within the MSD advertising data structure)
     * has the following composition:
     * 128-Bit UUID = E2 0A 39 F4 73 F5 4B C4 A1 2F 17 D1 AD 07 A9 61
     * Major/Minor  = 0000 / 0000
     * Tx Power     = C8 (-56dB)
     */
    const static uint8_t iBeaconPayload[] = {
        0x4C, 0x00, // Company identifier code (0x004C == Apple)
        0x02,       // ID
        0x15,       // length of the remaining payload
        0x5A, 0x4B, 0xCF, 0xCE, 0x17, 0x4E, 0x4B, 0xAC, // location UUID
        0xA8, 0x14, 0x09, 0x2E, 0x77, 0xF6, 0xB7, 0xE5,
        0x00, 0x1, // the major value to differentiate a location --1
        0x00, 0x1, // the minor value to differentiate a location --1
        0xC8        // 2's complement of the Tx power (-56dB)
    };

int main(void)
{
    /* Initialize BLE baselayer */
    ble.init();
    
    /* Set up iBeacon data*/
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE );
//ibeacon
    ble.accumulateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA, iBeaconPayload, sizeof(iBeaconPayload));
    
//    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
//                                            (const uint8_t *)SHORT_NAME,
//x                                             (sizeof(SHORT_NAME) - 1));
    //if (BLE_ERROR_NONE != err) {
    //    error(err, __LINE__);
    //}
 
    /* Set advertising interval. Longer interval = longer battery life */
    ble.setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);
    ble.setAdvertisingInterval(160); /* 100ms; in multiples of 0.625ms. */
    ble.startAdvertising();

    for (;;) {
        ble.waitForEvent(); 
    }
}
