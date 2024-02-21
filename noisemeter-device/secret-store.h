#include <mbedtls/aes.h>

class Secret
{
  constexpr static int BITS = 128;
  mbedtls_aes_context aes;
  unsigned char key[BITS / 8];

  void generateKey() {
    const auto id = String(DEVICE_ID);
    for (unsigned i = 0; i < sizeof(key); ++i)
      key[i] = id[i % id.length()];
  }

  String process(String in, int mode) {
    uint8_t out[64] = {0};
    mbedtls_aes_crypt_ecb(&aes, mode, (const uint8_t *)in.c_str(), out);
    return String((char *)out);
  }

public:
  Secret() {
    mbedtls_aes_init(&aes);
    generateKey();
  }

  ~Secret() {
    mbedtls_aes_free(&aes);
  }

  String encrypt(String in) {
    mbedtls_aes_setkey_enc(&aes, key, BITS);
    return process(in, MBEDTLS_AES_ENCRYPT);
  }

  String decrypt(String in) {
    mbedtls_aes_setkey_dec(&aes, key, BITS);
    return process(in, MBEDTLS_AES_DECRYPT);
  }
};


