Import("env")

env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.bin",
    env.VerboseAction(
        "openssl dgst -sha256 -sign priv_key.pem -keyform PEM -out $BUILD_DIR/${PROGNAME}.sig -binary $BUILD_DIR/${PROGNAME}.bin",
        "Creating OTA signature...")
)

env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.bin",
    env.VerboseAction(
        "cat $BUILD_DIR/${PROGNAME}.sig $BUILD_DIR/${PROGNAME}.bin > ${PROGNAME}_signed.bin",
        "Creating ${PROGNAME}_signed.bin")
)

