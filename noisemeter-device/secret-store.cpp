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
#include "secret-store.h"
#include "board.h"

#if defined(BOARD_ESP32_PCB)

#include <esp_hmac.h>
#include <mbedtls/aes.h>

void SecretStore::encrypt(const char *in, uint8_t *out, unsigned N) const noexcept
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    String k (key);
    {
        uint8_t hmac[BITS / 8];
        esp_hmac_calculate(HMAC_KEY0, k.c_str(), k.length(), hmac);
        mbedtls_aes_setkey_enc(&aes, hmac, BITS);
    }

    uint8_t iv[16];
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, N, iv,
        reinterpret_cast<const uint8_t *>(in), out);
}

void SecretStore::decrypt(const uint8_t *in, char *out, unsigned N) const noexcept
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    String k (key);
    {
        uint8_t hmac[BITS / 8];
        esp_hmac_calculate(HMAC_KEY0, k.c_str(), k.length(), hmac);
        mbedtls_aes_setkey_dec(&aes, hmac, BITS);
    }

    uint8_t iv[16];
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, N, iv,
        in, reinterpret_cast<uint8_t *>(out));
}

#else // !defined(BOARD_ESP32_PCB)

#include <algorithm>

void SecretStore::encrypt(const char *in, uint8_t *out, unsigned N) const noexcept
{
    std::copy(in, in + N, out);
}

void SecretStore::decrypt(const uint8_t *in, char *out, unsigned N) const noexcept
{
    std::copy(in, in + N, out);
}

#endif // defined(BOARD_ESP32_PCB)

