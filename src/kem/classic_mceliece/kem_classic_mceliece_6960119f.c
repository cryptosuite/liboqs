// SPDX-License-Identifier: MIT

#include <stdlib.h>

#include <oqs/kem_classic_mceliece.h>

#if defined(OQS_ENABLE_KEM_classic_mceliece_6960119f)

OQS_KEM *OQS_KEM_classic_mceliece_6960119f_new(void) {

	OQS_KEM *kem = malloc(sizeof(OQS_KEM));
	if (kem == NULL) {
		return NULL;
	}
	kem->method_name = OQS_KEM_alg_classic_mceliece_6960119f;
	kem->alg_version = "SUPERCOP-20191221";

	kem->claimed_nist_level = 5;
	kem->ind_cca = true;

	kem->length_public_key = OQS_KEM_classic_mceliece_6960119f_length_public_key;
	kem->length_secret_key = OQS_KEM_classic_mceliece_6960119f_length_secret_key;
	kem->length_ciphertext = OQS_KEM_classic_mceliece_6960119f_length_ciphertext;
	kem->length_shared_secret = OQS_KEM_classic_mceliece_6960119f_length_shared_secret;

	kem->keypair = OQS_KEM_classic_mceliece_6960119f_keypair;
	kem->encaps = OQS_KEM_classic_mceliece_6960119f_encaps;
	kem->decaps = OQS_KEM_classic_mceliece_6960119f_decaps;

	return kem;
}

extern int PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_keypair(uint8_t *pk, uint8_t *sk);
extern int PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
extern int PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

#if defined(OQS_ENABLE_KEM_classic_mceliece_6960119f_avx)
extern int PQCLEAN_MCELIECE6960119F_AVX_crypto_kem_keypair(uint8_t *pk, uint8_t *sk);
extern int PQCLEAN_MCELIECE6960119F_AVX_crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
extern int PQCLEAN_MCELIECE6960119F_AVX_crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
#endif

OQS_API OQS_STATUS OQS_KEM_classic_mceliece_6960119f_keypair(uint8_t *public_key, uint8_t *secret_key) {
#if defined(OQS_ENABLE_KEM_classic_mceliece_6960119f_avx)
#if defined(OQS_DIST_BUILD)
	if (OQS_CPU_has_extension(OQS_CPU_EXT_AVX2) && OQS_CPU_has_extension(OQS_CPU_EXT_BMI1) && OQS_CPU_has_extension(OQS_CPU_EXT_POPCNT)) {
#endif /* OQS_DIST_BUILD */
		return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_AVX_crypto_kem_keypair(public_key, secret_key);
#if defined(OQS_DIST_BUILD)
	} else {
		return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_keypair(public_key, secret_key);
	}
#endif /* OQS_DIST_BUILD */
#else
	return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_keypair(public_key, secret_key);
#endif
}

OQS_API OQS_STATUS OQS_KEM_classic_mceliece_6960119f_encaps(uint8_t *ciphertext, uint8_t *shared_secret, const uint8_t *public_key) {
#if defined(OQS_ENABLE_KEM_classic_mceliece_6960119f_avx)
#if defined(OQS_DIST_BUILD)
	if (OQS_CPU_has_extension(OQS_CPU_EXT_AVX2) && OQS_CPU_has_extension(OQS_CPU_EXT_BMI1) && OQS_CPU_has_extension(OQS_CPU_EXT_POPCNT)) {
#endif /* OQS_DIST_BUILD */
		return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_AVX_crypto_kem_enc(ciphertext, shared_secret, public_key);
#if defined(OQS_DIST_BUILD)
	} else {
		return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_enc(ciphertext, shared_secret, public_key);
	}
#endif /* OQS_DIST_BUILD */
#else
	return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_enc(ciphertext, shared_secret, public_key);
#endif
}

OQS_API OQS_STATUS OQS_KEM_classic_mceliece_6960119f_decaps(uint8_t *shared_secret, const uint8_t *ciphertext, const uint8_t *secret_key) {
#if defined(OQS_ENABLE_KEM_classic_mceliece_6960119f_avx)
#if defined(OQS_DIST_BUILD)
	if (OQS_CPU_has_extension(OQS_CPU_EXT_AVX2) && OQS_CPU_has_extension(OQS_CPU_EXT_BMI1) && OQS_CPU_has_extension(OQS_CPU_EXT_POPCNT)) {
#endif /* OQS_DIST_BUILD */
		return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_AVX_crypto_kem_dec(shared_secret, ciphertext, secret_key);
#if defined(OQS_DIST_BUILD)
	} else {
		return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_dec(shared_secret, ciphertext, secret_key);
	}
#endif /* OQS_DIST_BUILD */
#else
	return (OQS_STATUS) PQCLEAN_MCELIECE6960119F_VEC_crypto_kem_dec(shared_secret, ciphertext, secret_key);
#endif
}

#endif
