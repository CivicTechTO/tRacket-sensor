#ifndef SECRET_STORE_H
#define SECRET_STORE_H

#include <WString.h>

namespace Secret
{
    String encrypt(String key, String in);
    String decrypt(String key, String in);
}

#endif // SECRET_STORE_H

