/// @file
/// @brief Manages pesistent storage of WiFi credentials and other data
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

#include "secret-store.h"

#include <EEPROM.h>

#include <cstdint>

/**
 * Manages the storage of persistent settings.
 * At the moment, this is only used to store WiFi credentials.
 */
class Storage : protected EEPROMClass
{
    /** Maximum length of a stored String. */
    static constexpr unsigned StringSize = 64;

public:
    /** Tags to identify the stored settings. */
    enum class Entry : unsigned {
        Checksum  = 0,                           /** Storage CRC32 Checksum */
        SSID      = Checksum + sizeof(uint32_t), /** User's WiFi SSID */
        Passkey   = SSID     + StringSize,       /** User's WiFi passkey */
        Token     = Passkey  + StringSize,       /** Device API token */
        Email     = Token    + StringSize,       /** Temporary storage of user's email */
        TotalSize = Email    + StringSize        /** Marks storage end address */
    };

    /**
     * Initializes the instance and prepares flash memory for access.
     * @param key Key (i.e. seed) to use for encryption
     */
    void begin(UUID key);

    /**
     * Validates the stored settings against the stored checksum.
     * @return True if the validation is successful.
     */
    bool valid() const noexcept;

    /**
     * Checks if the given string can be stored via Storage.
     * @param str The string to check
     * @return True if the string can be stored
     */
    bool canStore(String str) const noexcept;

    /**
     * Clears/wipes all stored settings.
     */
    void clear() noexcept;

    /**
     * Gets the string value of the stored entry.
     * @param entry The storage entry to get
     * @return The string stored in the given entry
     */
    String get(Entry entry) const noexcept;

    /**
     * Sets the value of the given entry. Must call commit() to write to flash.
     * @param entry The storage entry to set
     * @param str The string to store in the entry
     */
    void set(Entry entry, String str) noexcept;

    /**
     * Commits all settings to flash and recalculates the checksum.
     */
    void commit() noexcept;

#ifdef STORAGE_SHOW_CREDENTIALS
    /**
     * Returns a string describing the stored settings.
     */
    operator String() const noexcept;
#endif

private:
    /** Encryption instance. */
    SecretStore secret;

    /**
     * Calculates a CRC32 checksum of all stored settings.
     * @return The checksum for the stored settings
     */
    uint32_t calculateChecksum() const noexcept;

    /**
     * Gets the memory address/offset of the given entry.
     * @param entry The entry to locate
     * @return The address of the entry within the storage address space
     */
    constexpr unsigned addrOf(Entry entry) const noexcept {
        return static_cast<unsigned>(entry);
    }
};

#endif // STORAGE_H

