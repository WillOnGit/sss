/* version number */
const char * const libsss_version;

/* encode and decode secrets <-> share files */
int sss_enc(const signed char * const inbuf, int n, FILE* sf[]);
int sss_dec(signed char * inbuf, int n, FILE *sf[]);
