#include <esp_hmac.h>
#include <mbedtls/aes.h>

class Secret
{
  constexpr static int BITS = 256; // do not change
  mbedtls_aes_context aes;
  uint8_t hmac[BITS / 8];

  bool generateKey(String key) {
    const auto result = esp_hmac_calculate(HMAC_KEY4, key.c_str(), key.length(), hmac);
    return result == ESP_OK;
  }

  String process(String in, int mode) {
    uint8_t out[64] = {0};
    mbedtls_aes_crypt_ecb(&aes, mode, (const uint8_t *)in.c_str(), out);
    return String((char *)out);
  }

public:
  Secret(String key) {
    mbedtls_aes_init(&aes);
    generateKey(key);
  }

  ~Secret() {
    mbedtls_aes_free(&aes);
  }

  String encrypt(String in) {
    mbedtls_aes_setkey_enc(&aes, hmac, BITS);
    return process(in, MBEDTLS_AES_ENCRYPT);
  }

  String decrypt(String in) {
    mbedtls_aes_setkey_dec(&aes, hmac, BITS);
    return process(in, MBEDTLS_AES_DECRYPT);
  }
};


