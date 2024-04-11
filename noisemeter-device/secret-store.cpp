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
#include "board.h"
#include "secret-store.h"

#if defined(BOARD_ESP32_PCB)

#include <esp_hmac.h>
#include <mbedtls/aes.h>

constexpr static unsigned BITS = 256; // do not change

namespace Secret {

String encrypt(String key, String in)
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    const auto kb = key.c_str();
    const auto kl = key.length();
    {
        uint8_t hmac[BITS / 8];
        esp_hmac_calculate(HMAC_KEY0, kb, kl, hmac);
        mbedtls_aes_setkey_enc(&aes, hmac, BITS);
    }

    char out[in.length()];
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT,
        reinterpret_cast<const uint8_t *>(in.c_str()),
        reinterpret_cast<uint8_t *>(out));
    return out;
}

String decrypt(String key, String in)
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    const auto kb = key.c_str();
    const auto kl = key.length();
    {
        uint8_t hmac[BITS / 8];
        esp_hmac_calculate(HMAC_KEY0, kb, kl, hmac);
        mbedtls_aes_setkey_dec(&aes, hmac, BITS);
    }

    char out[in.length()];
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT,
        reinterpret_cast<const uint8_t *>(in.c_str()),
        reinterpret_cast<uint8_t *>(out));
    return out;
}

} // namespace Secret

#else // !defined(BOARD_ESP32_PCB)

namespace Secret {

String encrypt([[maybe_unused]] String key, String in)
{
    return in;
}

String decrypt([[maybe_unused]] String key, String in)
{
    return in;
}

} // namespace Secret

#endif // defined(BOARD_ESP32_PCB)

