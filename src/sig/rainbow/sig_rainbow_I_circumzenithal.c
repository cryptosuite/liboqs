// SPDX-License-Identifier: MIT

#include <stdlib.h>

#include <oqs/sig_rainbow.h>

#if defined(OQS_ENABLE_SIG_rainbow_I_circumzenithal)

OQS_SIG *OQS_SIG_rainbow_I_circumzenithal_new(void) {

	OQS_SIG *sig = malloc(sizeof(OQS_SIG));
	if (sig == NULL) {
		return NULL;
	}
	sig->method_name = OQS_SIG_alg_rainbow_I_circumzenithal;
	sig->alg_version = "https://github.com/fast-crypto-lab/rainbow-submission-round2/commit/173ada0e077e1b9dbd8e4a78994f87acc0c92263";

	sig->claimed_nist_level = 1;
	sig->euf_cma = true;

	sig->length_public_key = OQS_SIG_rainbow_I_circumzenithal_length_public_key;
	sig->length_secret_key = OQS_SIG_rainbow_I_circumzenithal_length_secret_key;
	sig->length_signature = OQS_SIG_rainbow_I_circumzenithal_length_signature;

	sig->keypair = OQS_SIG_rainbow_I_circumzenithal_keypair;
	sig->sign = OQS_SIG_rainbow_I_circumzenithal_sign;
	sig->verify = OQS_SIG_rainbow_I_circumzenithal_verify;

	return sig;
}

extern int PQCLEAN_RAINBOWICIRCUMZENITHAL_CLEAN_crypto_sign_keypair(uint8_t *pk, uint8_t *sk);
extern int PQCLEAN_RAINBOWICIRCUMZENITHAL_CLEAN_crypto_sign_signature(uint8_t *sig, size_t *siglen, const uint8_t *m, size_t mlen, const uint8_t *sk);
extern int PQCLEAN_RAINBOWICIRCUMZENITHAL_CLEAN_crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m, size_t mlen, const uint8_t *pk);

OQS_API OQS_STATUS OQS_SIG_rainbow_I_circumzenithal_keypair(uint8_t *public_key, uint8_t *secret_key) {
	return (OQS_STATUS) PQCLEAN_RAINBOWICIRCUMZENITHAL_CLEAN_crypto_sign_keypair(public_key, secret_key);
}

OQS_API OQS_STATUS OQS_SIG_rainbow_I_circumzenithal_sign(uint8_t *signature, size_t *signature_len, const uint8_t *message, size_t message_len, const uint8_t *secret_key) {
	return (OQS_STATUS) PQCLEAN_RAINBOWICIRCUMZENITHAL_CLEAN_crypto_sign_signature(signature, signature_len, message, message_len, secret_key);
}

OQS_API OQS_STATUS OQS_SIG_rainbow_I_circumzenithal_verify(const uint8_t *message, size_t message_len, const uint8_t *signature, size_t signature_len, const uint8_t *public_key) {
	return (OQS_STATUS) PQCLEAN_RAINBOWICIRCUMZENITHAL_CLEAN_crypto_sign_verify(signature, signature_len, message, message_len, public_key);
}

#endif
