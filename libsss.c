#define SBUF_SIZE      32
#define LIBSSS_VERSION "2.0dev"
#define X_STR(x)       #x
#define STR(x)         X_STR(x)

#include <stdio.h>
#include <stdlib.h>

#include <gmp.h>

#include "libsss.h"

const char *const libsss_version = LIBSSS_VERSION
	"\ngmp version: " STR(__GNU_MP_VERSION) "." STR(
		__GNU_MP_VERSION_MINOR) "." STR(__GNU_MP_VERSION_PATCHLEVEL);

struct sss_share {
	mpz_t x;
	mpz_t y;
};

/*
 * return a global random state, initialising it from /dev/urandom if
 * necessary
 */
gmp_randstate_t *getstate()
{
	static gmp_randstate_t state;
	static int init = 0;

	unsigned int seed = 0;

	if (!init) {
		gmp_randinit_default(state);

		/* try to get random bytes for seed */
		FILE *urandom = fopen("/dev/urandom", "r");

		if (urandom == NULL) {
			fprintf(stderr,
			        "Can't open /dev/urandom!\nOutput shares will be unsafe.\n");
		} else {
			/*
			 * TODO: improve this random seeding
			 *
			 * This is just a project for fun so this will do for now
			 * but it would be interesting to revisit this.
			 *
			 * For instance, since sizeof(unsigned int) on this machine
			 * is 4, if I understand correctly there are only 2^32
			 * coefficients possible for this to generate (while we only
			 * generate one number, the coefficient a1), which is
			 * probably "easily" brute-forced.
			 *
			 * As far as I can tell the only alternative to seeding with
			 * an unsigned int is to seed with an mpz_t. But if you're
			 * already constructing an mpz_t from random bytes
			 * anyway, why not just directly create all your random
			 * numbers that way too?
			 *
			 * I guess it only makes a difference in future when we
			 * generate lots more numbers, so we save lots of reads.
			 * Maybe a cryptographer can weigh in on the pros and cons.
			 */
			for (int i = 0; i < sizeof(unsigned int); i++) {
				seed += getc(urandom) << (8 * i);
			}
		}
		fclose(urandom);

		/* ready to finish */
		gmp_randseed_ui(state, seed);
		init = 1;
		return &state;
	} else {
		/* random state already initialised */
		return &state;
	}
}

/*
 * serialise a share to a file, no NULL check
 */
int sss_ser(const struct sss_share *s, FILE *f)
{
	signed char xbytes[SBUF_SIZE] = { 0 };
	signed char ybytes[SBUF_SIZE] = { 0 };

	/*
	 * TODO: bounds checking
	 *
	 * I thought 32 bytes was always sufficient because the
	 * coordinates were bounded by 0 and 2^256-1, but actually they
	 * are bounded by 0 and p-1, which is 2^256 + 258.
	 *
	 * Like elsewhere it's hardly a critical bug but it should
	 * really be fixed just for correctness' sake.
	 */
	mpz_export(xbytes, NULL, -1, sizeof(char), 0, 0, s->x);
	mpz_export(ybytes, NULL, -1, sizeof(char), 0, 0, s->y);

	/*
	 * - magic bytes b6 94
	 * - serialisation format version
	 * - exactly 32 (SBUF_SIZE) bytes for x
	 * - exactly 32 (SBUF_SIZE) bytes for y
	 */
	fprintf(f, "\xb6\x94\x01");
	fwrite(xbytes, sizeof(char), SBUF_SIZE, f);
	fwrite(ybytes, sizeof(char), SBUF_SIZE, f);

	return 0;
}

/*
 * deserialise a share from a file
 *
 * does NOT check for NULL FILEs
 * expects (requires?) uninitialised shares
 */
int sss_des(FILE *f, struct sss_share *share)
{
	int c;
	static signed char inbuf[67];

	/* copy everything to inbuf and exit if error */
	for (int i = 0; i < 67; i++) {
		c = getc(f);
		if (c == EOF)
			return 1;

		inbuf[i] = c;
	}

	/* check magic */
	if (inbuf[0] != '\xb6' || inbuf[1] != '\x94')
		return 1;

	/* check version */
	if (inbuf[2] != '\x01')
		return 1;

	mpz_inits(share->x, share->y, NULL);
	mpz_import(share->x, SBUF_SIZE, -1, sizeof(char), 0, 0, &inbuf[3]);
	mpz_import(share->y, SBUF_SIZE, -1, sizeof(char), 0, 0, &inbuf[35]);

	return 0;
}

/*
 * generate shares of input 256-bit secret in (k, n) scheme, modulo
 * prime p = 115792089237316195423570985008687907853269984665640564039457584007913129640233
 *
 * return values:
 *     - 0: success
 *     - 1: invalid parameters
 */
int sss_enc(const signed char *const inbuf, int k, int n, FILE *sf[])
{
	gmp_randstate_t *state;
	mpz_t a[k - 1], p, x_pow, secret;
	struct sss_share shares[n];

	/* validation */
	if (n < 2 || k < 2 || n < k)
		return 1;

	/* init */
	mpz_inits(secret, x_pow, NULL);
	mpz_init_set_str(
		p,
		"115792089237316195423570985008687907853269984665640564039457584007913129640233",
		10);
	state = getstate();

	/* 0 <= secret < p */
	mpz_import(secret, SBUF_SIZE, -1, sizeof(char), 0, 0, inbuf);

	/* generate coefficients */
	for (int i = 0; i < k - 1; i++) {
		mpz_init(a[i]);
		mpz_urandomm(a[i], *state, p);
	}

	for (int i = 0; i < n; i++) {
		/*
		 * TODO: yes, this could result in some duplicate x
		 * coordinates but the chance is so vanishingly small it
		 * really doesn't feel worth it yet.
		 */
		mpz_init(shares[i].x);
		mpz_urandomm(shares[i].x, *state, p);
		mpz_init_set_ui(x_pow, 1);

		/* calculate y coordinate */
		mpz_init_set(shares[i].y, secret);

		/* could probably optimise the first iteration but cba */
		for (int j = 0; j < k - 1; j++) {
			/* next power of x */
			mpz_mul(x_pow, x_pow, shares[i].x);
			mpz_mod(x_pow, x_pow, p);

			/* multiply by coefficient and add to y */
			mpz_addmul(shares[i].y, a[j], x_pow);
			mpz_mod(shares[i].y, shares[i].y, p);
		}

		/* everything was OK; write share */
		sss_ser(&shares[i], sf[i]);
	}

	return 0;
}

/*
 * reconstruct a secret from a set of unique coordinates
 *
 * if shares with repeated x coordinates are given, probably just junk
 * data will come back but at worst there may be UB.
 */
void sss_rec(signed char *inbuf, int k, const struct sss_share shares[])
{
	mpz_t secret_sum, inner_prod, inner_term, p;

	/* don't modify inbuf on invalid k */
	if (k < 2)
		return;

	/* init */
	mpz_inits(inner_prod, inner_term, NULL);
	mpz_init_set_ui(secret_sum, 0);
	mpz_init_set_str(
		p,
		"115792089237316195423570985008687907853269984665640564039457584007913129640233",
		10);

	/* recover secret by directly evaluating k-order Lagrange polynomial at 0 */
	for (int outer = 0; outer < k; outer++) {
		mpz_set_ui(inner_prod, 1);

		/* calculate next product term and multiply into inner_prod */
		for (int inner = 0; inner < k; inner++) {
			if (inner == outer)
				continue;

			mpz_sub(inner_term, shares[inner].x,
			        shares[outer].x);
			mpz_invert(
				inner_term, inner_term,
				p); /* no need for mpz_mod before or after mpz_invert() */
			mpz_mul(inner_term, inner_term, shares[inner].x);

			mpz_mul(inner_prod, inner_prod, inner_term);
		}

		mpz_addmul(secret_sum, shares[outer].y, inner_prod);
	}

	/* ensure 0 <= secret < p and write to buffer */
	mpz_mod(secret_sum, secret_sum, p);
	mpz_export(inbuf, NULL, -1, sizeof(char), 0, 0, secret_sum);
}

/*
 * reconstruct secret from share files with k = 2.
 *
 * return values:
 *     - 0: success
 *     - 1: corrupt input share
 *     - 2: duplicated shares/x coordinates
 *     - 3: bad inputs
 */
int sss_dec(signed char *inbuf, int n, FILE *sf[])
{
	struct sss_share shares[n];

	if (n < 2)
		return 3;

	/* try to get a valid, unique share from each input file */
	for (int i = 0; i < n; i++) {
		int failed;
		failed = sss_des(sf[i], &shares[i]);

		/* check for well-formed share */
		if (failed) {
			return 1;
		}

		/* check for duplicate coordinates */
		for (int j = 0; j < i; j++) {
			if (!mpz_cmp(shares[i].x, shares[j].x)) {
				return 2;
			}
		}
	}

	/* everything OK, proceed */
	sss_rec(inbuf, n, shares);
	return 0;
}
