/* version number */
const char * const sss_libver;

/* data type */
struct sss_share;

/* encode and decode secrets <-> shares */
int sss_enc(const signed char * const inbuf, FILE *sf1, FILE *sf2, FILE *sf3);
int sss_dec(signed char * inbuf, FILE *s1, FILE *s2);

/* serialise and deserialise shares <-> files */
int sss_ser(const struct sss_share *s, FILE *f);
struct sss_share *sss_des(FILE *f);
