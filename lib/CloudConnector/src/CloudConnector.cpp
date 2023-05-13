#include "CloudConnector.h"

CloudConnector::CloudConnector(RemoteDebug* debugger) {
    this->debugger = debugger;
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    int error_code = 0;
    if((error_code =  mbedtls_pk_parse_public_key(&pk, PUBLIC_KEY, sizeof(PUBLIC_KEY))) == 0){
        debugger->printf("RSA public key OK\n");
        rsa = mbedtls_pk_rsa(pk);
    } else {
        debugger->printf("RSA public key read error: ");
        mbedtls_strerror(error_code, (char*) buf, 4096);
        debugger->printf("%s\n", buf);
    }
    debugger->flush();
    //send();
}

void CloudConnector::send() {
    if(rsa != nullptr && mbedtls_rsa_check_pubkey(rsa) == 0) {
        memset(buf, 0, 4096);

        CloudData data = {65, 127};
        unsigned char toEncrypt[4096] = {0};

        debugger->println("RSA clear data: ");
        debugPrint(toEncrypt, 0, 256);

        mbedtls_rsa_rsaes_pkcs1_v15_encrypt(rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, 256, toEncrypt, buf);

        //byte hashResult[32];
        //mbedtls_sha256(toEncrypt, strlen((char*) toEncrypt), hashResult, 0);
        //int success = mbedtls_rsa_rsassa_pkcs1_v15_sign(rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256,  strlen((char*) hashResult), hashResult, buf);
        debugger->println("RSA encrypted data: ");
        debugPrint(buf, 0, 256);
    } else {
        debugger->println("RSA key is invalid");
    }
}
void CloudConnector::debugPrint(byte *buffer, int start, int length) {
	for (int i = start; i < start + length; i++) {
		if (buffer[i] < 0x10)
			debugger->print(F("0"));
		debugger->print(buffer[i], HEX);
		debugger->print(F(" "));
		if ((i - start + 1) % 16 == 0)
			debugger->println(F(""));
		else if ((i - start + 1) % 4 == 0)
			debugger->print(F(" "));

		yield(); // Let other get some resources too
	}
	debugger->println(F(""));
}
