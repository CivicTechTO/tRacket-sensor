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

#include <WString.h>

namespace Secret
{
    /**
     * Encrypts the given string with the given key.
     * Uses the HMAC peripheral to modify the key in a secure way.
     * @param key Key to use for encryption.
     * @param in String of data to be encrypted.
     * @return The encrypted string.
     */
    String encrypt(String key, String in);

    /**
     * Decrypts the given string with the given key.
     * Uses the HMAC peripheral to modify the key in a secure way.
     * @param key Key to use for decryption.
     * @param in String of data to be decrypted.
     * @return The decrypted string.
     */
    String decrypt(String key, String in);
}

#endif // SECRET_STORE_H

