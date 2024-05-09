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
#include "storage.h"

#include <Arduino.h>
#include <CRC32.h>

#include <algorithm>

void Storage::begin() noexcept
{
    EEPROMClass::begin(addrOf(Entry::TotalSize));
    delay(2000);  // Ensure the eeprom peripheral has enough time to initialize.
}

bool Storage::valid() const noexcept
{
    const auto calc = calculateChecksum();
    const auto addr = _data + addrOf(Entry::Checksum);
    const auto stored = *reinterpret_cast<uint32_t *>(addr);
    return stored == calc;
}

bool Storage::canStore(String str) const noexcept
{
    return str.length() < StringSize;
}

void Storage::clear() noexcept
{
    for (auto i = 0u; i < addrOf(Entry::TotalSize); ++i)
        writeByte(i, 0xFF);

    EEPROMClass::commit();
}

String Storage::get(Entry entry) const noexcept
{
    if (entry != Entry::Checksum)
        return String(_data + addrOf(entry), StringSize - 1);
    else
        return {};
}

void Storage::set(Entry entry, String str) noexcept
{
    if (entry != Entry::Checksum && canStore(str))
        writeBytes(addrOf(entry), str.begin(), StringSize);
}

void Storage::commit() noexcept
{
    const auto csum = calculateChecksum();
    writeUInt(addrOf(Entry::Checksum), csum);
    EEPROMClass::commit();
}

Storage::operator String() const noexcept
{
    return String() +
           "SSID \"" + get(Entry::SSID) +
#ifdef STORAGE_SHOW_PASSKEY
           "\" Passkey \"" + get(Entry::Passkey) +
#endif
           '\"';
}

uint32_t Storage::calculateChecksum() const noexcept
{
    const auto addr = _data + sizeof(uint32_t);
    const auto size = addrOf(Entry::TotalSize) - sizeof(uint32_t);
    return CRC32::calculate(addr, size);
}

