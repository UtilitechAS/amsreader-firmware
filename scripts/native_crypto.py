"""PlatformIO pre-build hook for the native test env.

Probes for mbedTLS development headers. If found, defines HAVE_MBEDTLS and
links libmbedcrypto so GcmParser can decrypt on host; otherwise the build
falls back to the no-crypto stub and encrypted tests self-ignore.

Install headers (Debian/Ubuntu): sudo apt-get install -y libmbedtls-dev
"""
import os

Import("env")  # noqa: F821  (injected by PlatformIO/SCons)

SEARCH = ["/usr/include", "/usr/local/include", "/opt/homebrew/include", "/usr/local/opt/mbedtls/include"]
hdr = next((p for p in SEARCH if os.path.exists(os.path.join(p, "mbedtls", "gcm.h"))), None)

if hdr:
    print("native_crypto: mbedTLS found at %s -> enabling HAVE_MBEDTLS" % hdr)
    env.Append(CPPDEFINES=["HAVE_MBEDTLS"], CPPPATH=[hdr], LIBS=["mbedcrypto"])
    libdir = os.path.join(os.path.dirname(hdr), "lib")
    if os.path.isdir(libdir):
        env.Append(LIBPATH=[libdir])
else:
    print("native_crypto: mbedTLS headers not found; encrypted-decode tests will be skipped.")
    print("               install with: sudo apt-get install -y libmbedtls-dev")
