Import("env")

env.AddCustomTarget(
    name="ota",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=[
        "openssl dgst -sha256 -sign priv_key.pem -keyform PEM -out $BUILD_DIR/${PROGNAME}.sig -binary $BUILD_DIR/${PROGNAME}.bin",
        "cat $BUILD_DIR/${PROGNAME}.sig $BUILD_DIR/${PROGNAME}.bin > ${PROGNAME}_signed.bin"
    ],
    title="OTA Signing",
    description="Create a signed OTA update"
)

