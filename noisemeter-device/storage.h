/* noisemeter-device - Firmware for CivicTechTO's Noisemeter Device
 * Copyright (C) 2024  Clyne Sullivan, Nick Barnard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef STORAGE_H
#define STORAGE_H

#include <EEPROM.h>

#include <cstdint>

/**
 * Manages the storage of persistent settings e.g. WiFi credentials.
 */
class Storage : protected EEPROMClass
{
    static constexpr unsigned StringSize = 64;

public:
    enum class Entry : unsigned {
        Checksum  = 0,
        SSID      = Checksum + sizeof(uint32_t),
        Passkey   = SSID     + StringSize,
        TotalSize = Passkey  + StringSize
    };

    // Prepares flash memory for access.
    void begin() noexcept;

    // Returns true if the stored data matches its checksum.
    bool valid() const noexcept;

    // Returns true if the given string can fit in a string entry.
    bool canStore(String str) const noexcept;

    // Clears/wipes all stored settings.
    void clear() noexcept;

    // Gets the string value of the stored entry.
    String get(Entry entry) const noexcept;

    // Sets the value of the given entry. Must call commit() to write to flash.
    void set(Entry entry, String str) noexcept;

    // Commits all settings to flash and recalculates the checksum.
    void commit() noexcept;

    // Returns a string describing the stored settings.
    operator String() const noexcept;

private:
    // Calculates a CRC32 checksum of all stored settings.
    uint32_t calculateChecksum() const noexcept;

    // Gets the memory address/offset of the given entry.
    constexpr unsigned addrOf(Entry entry) const noexcept {
        return static_cast<unsigned>(entry);
    }
};

#endif // STORAGE_H

