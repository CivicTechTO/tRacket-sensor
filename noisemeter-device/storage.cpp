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
    if (entry != Entry::Checksum) {
        char buffer[StringSize];
        const auto start = _data + addrOf(entry);
        auto end = std::copy(start, start + sizeof(buffer) - 1, buffer);
        *end = '\0';
        return buffer;
    } else {
        return {};
    }
}

void Storage::set(Entry entry, String str) noexcept
{
    if (entry != Entry::Checksum && canStore(str))
        writeString(addrOf(entry), str);
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

