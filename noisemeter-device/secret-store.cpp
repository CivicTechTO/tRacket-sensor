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

