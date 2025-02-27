#include "NTT_params.h"
#include "indcpa.h"
#include "ntt.h"
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "randombytes.h"
#include "rejsample.h"
#include "symmetric.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*************************************************
* Name:        pack_pk
*
* Description: Serialize the public key as concatenation of the
*              serialized vector of polynomials pk
*              and the public seed used to generate the matrix A.
*
* Arguments:   uint8_t *r: pointer to the output serialized public key
*              polyvec *pk: pointer to the input public-key polyvec
*              const uint8_t *seed: pointer to the input public seed
**************************************************/
static void pack_pk(uint8_t r[KYBER_INDCPA_PUBLICKEYBYTES],
                    int16_t pk[KYBER_K][KYBER_N],
                    const uint8_t seed[KYBER_SYMBYTES]) {
    size_t i;
    polyvec_tobytes(r, pk);
    for (i = 0; i < KYBER_SYMBYTES; i++) {
        r[i + KYBER_POLYVECBYTES] = seed[i];
    }
}

/*************************************************
* Name:        unpack_pk
*
* Description: De-serialize public key from a byte array;
*              approximate inverse of pack_pk
*
* Arguments:   - polyvec *pk: pointer to output public-key polynomial vector
*              - uint8_t *seed: pointer to output seed to generate matrix A
*              - const uint8_t *packedpk: pointer to input serialized public key
**************************************************/
static void unpack_pk(int16_t pk[KYBER_K][KYBER_N],
                      uint8_t seed[KYBER_SYMBYTES],
                      const uint8_t packedpk[KYBER_INDCPA_PUBLICKEYBYTES]) {
    size_t i;
    polyvec_frombytes(pk, packedpk);
    for (i = 0; i < KYBER_SYMBYTES; i++) {
        seed[i] = packedpk[i + KYBER_POLYVECBYTES];
    }
}

/*************************************************
* Name:        pack_sk
*
* Description: Serialize the secret key
*
* Arguments:   - uint8_t *r: pointer to output serialized secret key
*              - polyvec *sk: pointer to input vector of polynomials (secret key)
**************************************************/
static void pack_sk(uint8_t r[KYBER_INDCPA_SECRETKEYBYTES], int16_t sk[KYBER_K][KYBER_N]) {
    polyvec_tobytes(r, sk);
}

/*************************************************
* Name:        unpack_sk
*
* Description: De-serialize the secret key; inverse of pack_sk
*
* Arguments:   - polyvec *sk: pointer to output vector of polynomials (secret key)
*              - const uint8_t *packedsk: pointer to input serialized secret key
**************************************************/
static void unpack_sk(int16_t sk[KYBER_K][KYBER_N], const uint8_t packedsk[KYBER_INDCPA_SECRETKEYBYTES]) {
    polyvec_frombytes(sk, packedsk);
}

/*************************************************
* Name:        pack_ciphertext
*
* Description: Serialize the ciphertext as concatenation of the
*              compressed and serialized vector of polynomials b
*              and the compressed and serialized polynomial v
*
* Arguments:   uint8_t *r: pointer to the output serialized ciphertext
*              poly *pk: pointer to the input vector of polynomials b
*              poly *v: pointer to the input polynomial v
**************************************************/
static void pack_ciphertext(uint8_t r[KYBER_INDCPA_BYTES], int16_t b[KYBER_K][KYBER_N], int16_t *v) {
    polyvec_compress(r, b);
    poly_compress(r + KYBER_POLYVECCOMPRESSEDBYTES, v);
}

/*************************************************
* Name:        unpack_ciphertext
*
* Description: De-serialize and decompress ciphertext from a byte array;
*              approximate inverse of pack_ciphertext
*
* Arguments:   - polyvec *b: pointer to the output vector of polynomials b
*              - poly *v: pointer to the output polynomial v
*              - const uint8_t *c: pointer to the input serialized ciphertext
**************************************************/
static void unpack_ciphertext(int16_t b[KYBER_K][KYBER_N], int16_t *v, const uint8_t c[KYBER_INDCPA_BYTES]) {
    polyvec_decompress(b, c);
    poly_decompress(v, c + KYBER_POLYVECCOMPRESSEDBYTES);
}


#define gen_a(A,B)  gen_matrix(A,B,0)
#define gen_at(A,B) gen_matrix(A,B,1)

/*************************************************
* Name:        gen_matrix
*
* Description: Deterministically generate matrix A (or the transpose of A)
*              from a seed. Entries of the matrix are polynomials that look
*              uniformly random. Performs rejection sampling on output of
*              a XOF
*
* Arguments:   - polyvec *a: pointer to ouptput matrix A
*              - const uint8_t *seed: pointer to input seed
*              - int transposed: boolean deciding whether A or A^T is generated
**************************************************/
#define GEN_MATRIX_NBLOCKS ((12*KYBER_N/8*(1 << 12)/KYBER_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)
// Not static for benchmarking
void gen_matrix(int16_t a[KYBER_K][KYBER_K][KYBER_N], const uint8_t seed[KYBER_SYMBYTES], int transposed) {
    unsigned int ctr0, ctr1, k;
    unsigned int buflen, off;
    uint8_t buf0[GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES + 2],
            buf1[GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES + 2];
    neon_xof_state state;

    int16_t *s1 = NULL, *s2 = NULL;
    unsigned int x1, x2, y1, y2;
    xof_state c_state;
    xof_init(&c_state);

    for (unsigned int j = 0; j < KYBER_K * KYBER_K - 1; j += 2) {
        switch (j) {
        case 0:
            s1 = &(a[0][0][0]);
            s2 = &(a[0][1][0]);
            x1 = 0;
            y1 = 0;
            x2 = 0;
            y2 = 1;
            break;
        case 2:
            s1 = &(a[0][2][0]);
            s2 = &(a[1][0][0]);
            x1 = 0;
            y1 = 2;
            x2 = 1;
            y2 = 0;
            break;
        case 4:
            s1 = &(a[1][1][0]);
            s2 = &(a[1][2][0]);
            x1 = 1;
            y1 = 1;
            x2 = 1;
            y2 = 2;
            break;
        default:
            s1 = &(a[2][0][0]);
            s2 = &(a[2][1][0]);
            x1 = 2;
            y1 = 0;
            x2 = 2;
            y2 = 1;
            break;
        }

        if (transposed) {
            neon_xof_absorb(&state, seed, x1, x2, y1, y2);
        } else {
            neon_xof_absorb(&state, seed, y1, y2, x1, x2);
        }

        neon_xof_squeezeblocks(buf0, buf1, GEN_MATRIX_NBLOCKS, &state);

        buflen = GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES;

        ctr0 = neon_rej_uniform(s1, buf0);
        ctr1 = neon_rej_uniform(s2, buf1);

        while (ctr0 < KYBER_N || ctr1 < KYBER_N) {
            off = buflen % 3;
            for (k = 0; k < off; k++) {
                buf0[k] = buf0[buflen - off + k];
                buf1[k] = buf1[buflen - off + k];
            }
            neon_xof_squeezeblocks(buf0 + off, buf1 + off, 1, &state);

            buflen = off + XOF_BLOCKBYTES;
            ctr0 += rej_uniform(s1 + ctr0, KYBER_N - ctr0, buf0, buflen);
            ctr1 += rej_uniform(s2 + ctr1, KYBER_N - ctr1, buf1, buflen);
        }
    }

    // Last iteration [2][2]
    if (transposed) {
        xof_absorb(&c_state, seed, 2, 2);
    } else {
        xof_absorb(&c_state, seed, 2, 2);
    }

    xof_squeezeblocks(buf0, GEN_MATRIX_NBLOCKS, &c_state);

    buflen = GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES;

    ctr0 = neon_rej_uniform(&(a[2][2][0]), buf0);

    while (ctr0 < KYBER_N) {
        off = buflen % 3;
        for (k = 0; k < off; k++) {
            buf0[k] = buf0[buflen - off + k];
        }
        xof_squeezeblocks(buf0 + off, 1, &c_state);

        buflen = off + XOF_BLOCKBYTES;
        ctr0 += rej_uniform(&(a[2][2][0]) + ctr0, KYBER_N - ctr0, buf0, buflen);
    }
    shake128_inc_ctx_release(&c_state);

}

/*************************************************
* Name:        indcpa_keypair
*
* Description: Generates public and private key for the CPA-secure
*              public-key encryption scheme underlying Kyber
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                             (of length KYBER_INDCPA_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
                              (of length KYBER_INDCPA_SECRETKEYBYTES bytes)
**************************************************/
void indcpa_keypair(uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                    uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES]) {
    unsigned int i;
    uint8_t buf[2 * KYBER_SYMBYTES];
    const uint8_t *publicseed = buf;
    const uint8_t *noiseseed = buf + KYBER_SYMBYTES;
    int16_t a[KYBER_K][KYBER_K][KYBER_N];
    int16_t e[KYBER_K][KYBER_N];
    int16_t pkpv[KYBER_K][KYBER_N];
    int16_t skpv[KYBER_K][KYBER_N];
    int16_t skpv_asymmetric[KYBER_K][KYBER_N >> 1];

    randombytes(buf, KYBER_SYMBYTES);
    hash_g(buf, buf, KYBER_SYMBYTES);

    gen_a(a, publicseed);

    neon_poly_getnoise_eta1_2x(&(skpv[0][0]), &(skpv[1][0]), noiseseed, 0, 1);
    neon_poly_getnoise_eta1_2x(&(skpv[2][0]), &(e[0][0]), noiseseed, 2, 3);
    neon_poly_getnoise_eta1_2x(&(e[1][0]), &(e[2][0]), noiseseed, 4, 5);

    neon_polyvec_ntt(skpv);
    neon_polyvec_ntt(e);

    for (i = 0; i < KYBER_K; i++) {
        PQCLEAN_KYBER768_AARCH64_asm_point_mul_extended(&(skpv_asymmetric[i][0]), &(skpv[i][0]), pre_asymmetric_table_Q1_extended, asymmetric_const);
    }

    for (i = 0; i < KYBER_K; i++) {
        PQCLEAN_KYBER768_AARCH64_asm_asymmetric_mul_montgomery(&(a[i][0][0]), &(skpv[0][0]), &(skpv_asymmetric[0][0]), asymmetric_const, pkpv[i]);
    }

    neon_polyvec_add_reduce(pkpv, e);

    pack_sk(sk, skpv);
    pack_pk(pk, pkpv, publicseed);
}

void indcpa_keypair_with_recovery(uint8_t seed[KYBER_SYMBYTES],
                    bool recovery,
                    uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                    uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES]) {
    unsigned int i;
    uint8_t buf[2 * KYBER_SYMBYTES];
    const uint8_t *publicseed = buf;
    const uint8_t *noiseseed = buf + KYBER_SYMBYTES;
    int16_t a[KYBER_K][KYBER_K][KYBER_N];
    int16_t e[KYBER_K][KYBER_N];
    int16_t pkpv[KYBER_K][KYBER_N];
    int16_t skpv[KYBER_K][KYBER_N];
    int16_t skpv_asymmetric[KYBER_K][KYBER_N >> 1];

    if (!recovery){
        randombytes(buf, KYBER_SYMBYTES);
        memcpy(seed,buf,KYBER_SYMBYTES);
    }else{
        memcpy(buf,seed,KYBER_SYMBYTES);
    }
    hash_g(buf, buf, KYBER_SYMBYTES);

    gen_a(a, publicseed);

    neon_poly_getnoise_eta1_2x(&(skpv[0][0]), &(skpv[1][0]), noiseseed, 0, 1);
    neon_poly_getnoise_eta1_2x(&(skpv[2][0]), &(e[0][0]), noiseseed, 2, 3);
    neon_poly_getnoise_eta1_2x(&(e[1][0]), &(e[2][0]), noiseseed, 4, 5);

    neon_polyvec_ntt(skpv);
    neon_polyvec_ntt(e);

    for (i = 0; i < KYBER_K; i++) {
        PQCLEAN_KYBER768_AARCH64_asm_point_mul_extended(&(skpv_asymmetric[i][0]), &(skpv[i][0]), pre_asymmetric_table_Q1_extended, asymmetric_const);
    }

    for (i = 0; i < KYBER_K; i++) {
        PQCLEAN_KYBER768_AARCH64_asm_asymmetric_mul_montgomery(&(a[i][0][0]), &(skpv[0][0]), &(skpv_asymmetric[0][0]), asymmetric_const, pkpv[i]);
    }

    neon_polyvec_add_reduce(pkpv, e);

    pack_sk(sk, skpv);
    pack_pk(pk, pkpv, publicseed);
}

/*************************************************
* Name:        indcpa_enc
*
* Description: Encryption function of the CPA-secure
*              public-key encryption scheme underlying Kyber.
*
* Arguments:   - uint8_t *c:           pointer to output ciphertext
*                                      (of length KYBER_INDCPA_BYTES bytes)
*              - const uint8_t *m:     pointer to input message
*                                      (of length KYBER_INDCPA_MSGBYTES bytes)
*              - const uint8_t *pk:    pointer to input public key
*                                      (of length KYBER_INDCPA_PUBLICKEYBYTES)
*              - const uint8_t *coins: pointer to input random coins
*                                      used as seed (of length KYBER_SYMBYTES)
*                                      to deterministically generate all
*                                      randomness
**************************************************/
void indcpa_enc(uint8_t c[KYBER_INDCPA_BYTES],
                const uint8_t m[KYBER_INDCPA_MSGBYTES],
                const uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                const uint8_t coins[KYBER_SYMBYTES]) {
    unsigned int i;
    uint8_t seed[KYBER_SYMBYTES];
    int16_t at[KYBER_K][KYBER_K][KYBER_N];
    int16_t sp[KYBER_K][KYBER_N];
    int16_t sp_asymmetric[KYBER_K][KYBER_N >> 1];
    int16_t pkpv[KYBER_K][KYBER_N];
    int16_t ep[KYBER_K][KYBER_N];
    int16_t b[KYBER_K][KYBER_N];
    int16_t v[KYBER_N];
    int16_t k[KYBER_N];
    int16_t epp[KYBER_N];

    unpack_pk(pkpv, seed, pk);
    poly_frommsg(k, m);
    gen_at(at, seed);

    // Because ETA1 == ETA2
    neon_poly_getnoise_eta1_2x(&(sp[0][0]), &(sp[1][0]), coins, 0, 1);
    neon_poly_getnoise_eta1_2x(&(sp[2][0]), &(ep[0][0]), coins, 2, 3);
    neon_poly_getnoise_eta1_2x(&(ep[1][0]), &(ep[2][0]), coins, 4, 5);
    neon_poly_getnoise_eta2(&(epp[0]), coins, 6);

    neon_polyvec_ntt(sp);

    for (i = 0; i < KYBER_K; i++) {
        PQCLEAN_KYBER768_AARCH64_asm_point_mul_extended(&(sp_asymmetric[i][0]), &(sp[i][0]), pre_asymmetric_table_Q1_extended, asymmetric_const);
    }

    for (i = 0; i < KYBER_K; i++) {
        PQCLEAN_KYBER768_AARCH64_asm_asymmetric_mul(&(at[i][0][0]), &(sp[0][0]), &(sp_asymmetric[0][0]), asymmetric_const, b[i]);
    }

    PQCLEAN_KYBER768_AARCH64_asm_asymmetric_mul(&(pkpv[0][0]), &(sp[0][0]), &(sp_asymmetric[0][0]), asymmetric_const, v);

    neon_polyvec_invntt_to_mont(b);
    invntt(v);

    neon_polyvec_add_reduce(b, ep);

    neon_poly_add_add_reduce(v, epp, k);

    pack_ciphertext(c, b, v);
}

/*************************************************
* Name:        indcpa_dec
*
* Description: Decryption function of the CPA-secure
*              public-key encryption scheme underlying Kyber.
*
* Arguments:   - uint8_t *m:        pointer to output decrypted message
*                                   (of length KYBER_INDCPA_MSGBYTES)
*              - const uint8_t *c:  pointer to input ciphertext
*                                   (of length KYBER_INDCPA_BYTES)
*              - const uint8_t *sk: pointer to input secret key
*                                   (of length KYBER_INDCPA_SECRETKEYBYTES)
**************************************************/
void indcpa_dec(uint8_t m[KYBER_INDCPA_MSGBYTES],
                const uint8_t c[KYBER_INDCPA_BYTES],
                const uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES]) {
    unsigned int i;
    int16_t b[KYBER_K][KYBER_N];
    int16_t b_asymmetric[KYBER_K][KYBER_N >> 1];
    int16_t skpv[KYBER_K][KYBER_N];
    int16_t v[KYBER_N];
    int16_t mp[KYBER_N];


    unpack_ciphertext(b, v, c);
    unpack_sk(skpv, sk);

    neon_polyvec_ntt(b);

    for (i = 0; i < KYBER_K; i++) {
        PQCLEAN_KYBER768_AARCH64_asm_point_mul_extended(&(b_asymmetric[i][0]), &(b[i][0]), pre_asymmetric_table_Q1_extended, asymmetric_const);
    }

    PQCLEAN_KYBER768_AARCH64_asm_asymmetric_mul(&(skpv[0][0]), &(b[0][0]), &(b_asymmetric[0][0]), asymmetric_const, mp);

    invntt(mp);

    neon_poly_sub_reduce(v, mp);

    poly_tomsg(m, v);
}
