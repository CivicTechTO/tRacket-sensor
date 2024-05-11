/// @file
/// @brief Facilities for encrypting sensitive stored data
/* noisemeter-device - Firmware for CivicTechTO's Noisemeter Device
 * Copyright (C) 2024  Clyne Sullivan
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
#ifndef SECRET_STORE_H
#define SECRET_STORE_H

#include "UUID/UUID.h"

struct SecretStore
{
    UUID key;

    /**
     * Securely encrypts the given string.
     * @param in Pointer to `len` bytes of input data.
     * @param out Pre-allocated buffer to store encrypted output.
     * @param len Number of bytes to process.
     */
    void encrypt(const char *in, uint8_t *out, unsigned int len) const noexcept;

    /**
     * Securely decrypts the given string.
     * @param in Pointer to `len` bytes of input data.
     * @param out Pre-allocated buffer to store decrypted output.
     * @param len Number of bytes to process.
     */
    void decrypt(const uint8_t *in, char *out, unsigned int len) const noexcept;

private:
    constexpr static unsigned BITS = 256; // do not change
};

#endif // SECRET_STORE_H

