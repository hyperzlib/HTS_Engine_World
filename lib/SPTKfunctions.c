
#include "SPTKfunctions.h"
#include "SPTK.h"
#include <stdlib.h>
#include  <stdio.h>
#include <math.h>



void mgc2sp(double *mgc, const int m, const double a, const double g, double *x,
            double *y, const int flng)
{
	int i;
	/*for (i=0;i<423*35;i++){
	    		printf("%f\n",mgc[i]);
	    }*/
	/*printf("%d\n",m);
	printf("%f\n",a);
	printf("%f\n",g);
	printf("%d\n",flng);
	printf("%d\n",sizeof(*x));*/
	/*for (i=0;i<sizeof(*x);i++){
		    		printf("%f\n",x[i]);
		    }*/

    static double *c = NULL;
    static int size;
    
    if (c == NULL) {
        c = dgetmem(flng / 2 + 1);
        size = flng;
    }
    if (flng > size) {
        free(c);
        c = dgetmem(flng / 2 + 1);
        size = flng;
    }
    
    mgc2mgc(mgc, m, a, g, c, flng / 2, 0.0, 0.0);
    c2sp(c, flng / 2, x, y, flng);
    
    return;
}

void mgc2mgc(double *c1, const int m1, const double a1, const double g1,
             double *c2, const int m2, const double a2, const double g2)
{
    double a;
    static double *ca = NULL;
    static int size_a;
    
    if (ca == NULL) {
        ca = dgetmem(m1 + 1);
        size_a = m1;
    }
    if (m1 > size_a) {
        free(ca);
        ca = dgetmem(m1 + 1);
        size_a = m1;
    }
    
    a = (a2 - a1) / (1 - a1 * a2);
    
    if (a == 0) {
        movem(c1, ca, sizeof(*c1), m1 + 1);
        gnorm(ca, ca, m1, g1);
        gc2gc(ca, m1, g1, c2, m2, g2);
        ignorm(c2, c2, m2, g2);
    } else {
        freqt(c1, m1, c2, m2, a);
        gnorm(c2, c2, m2, g1);
        gc2gc(c2, m2, g1, c2, m2, g2);
        ignorm(c2, c2, m2, g2);
    }
    int i;

    return;
}

void c2sp(double *c, const int m, double *x, double *y, const int l)
{
    int m1;
    
    m1 = m + 1;
    
    movem(c, x, sizeof(*c), m1);

    fillz(x + m1, sizeof(*x), l - m1);
    
    fftr(x, y, l);
    int i;
    /*for (i=0;i<100;i++){
       		printf("%f\n",x[i]);
       }
    for (i=0;i<100;i++){
          		printf("%f\n",y[i]);
          }*/
}

double *dgetmem(const int leng)
{
    return ((double *) getmem((size_t) leng, sizeof(double)));
}



void movem(void *a, void *b, const size_t size, const int nitem)
{
    long i;
    char *c = a;
    char *d = b;
    
    i = size * nitem;
    if (c > d)
        while (i--)
            *d++ = *c++;
    else {
        c += i;
        d += i;
        while (i--)
            *--d = *--c;
    }
}


char *getmem(const size_t leng, const size_t size)
{
    char *p = NULL;
    
    if ((p = (char *) calloc(leng, size)) == NULL) {
        fprintf(stderr, "Cannot allocate memory!\n");
        exit(3);
    }
    return (p);
}

void gnorm(double *c1, double *c2, int m, const double g)
{
    double k;
    
    if (g != 0.0) {
        k = 1.0 + g * c1[0];
        for (; m >= 1; m--)
            c2[m] = c1[m] / k;
        c2[0] = pow(k, 1.0 / g);
    } else {
        movem(&c1[1], &c2[1], sizeof(*c1), m);
        c2[0] = exp(c1[0]);
    }
    
    return;
}

void ignorm(double *c1, double *c2, int m, const double g)
{
    double k;
    
    k = pow(c1[0], g);
    if (g != 0.0) {
        for (; m >= 1; m--)
            c2[m] = k * c1[m];
        c2[0] = (k - 1.0) / g;
    } else {
        movem(&c1[1], &c2[1], sizeof(*c1), m);
        c2[0] = log(c1[0]);
    }
    
    return;
}

void gc2gc(double *c1, const int m1, const double g1, double *c2, const int m2,
           const double g2)
{
    int i, min, k, mk;
    double ss1, ss2, cc;
    static double *ca = NULL;
    static int size;
    
    if (ca == NULL) {
        ca = dgetmem(m1 + 1);
        size = m1;
    }
    if (m1 > size) {
        free(ca);
        ca = dgetmem(m1 + 1);
        size = m1;
    }
    
    movem(c1, ca, sizeof(*c1), m1 + 1);
    
    c2[0] = ca[0];
    for (i = 1; i <= m2; i++) {
        ss1 = ss2 = 0.0;
        min = (m1 < i) ? m1 : i - 1;
        for (k = 1; k <= min; k++) {
            mk = i - k;
            cc = ca[k] * c2[mk];
            ss2 += k * cc;
            ss1 += mk * cc;
        }
        
        if (i <= m1)
            c2[i] = ca[i] + (g2 * ss2 - g1 * ss1) / i;
        else
            c2[i] = (g2 * ss2 - g1 * ss1) / i;
    }
    
    return;
}


extern double *_sintbl;
extern int maxfftsize;

int fftr(double *x, double *y, const int m)
{
    int i, j;
    double *xp, *yp, *xq;
    double *yq;
    int mv2, n, tblsize;
    double xt, yt, *sinp, *cosp;
    double arg;
    
    mv2 = m / 2;
    
    /* separate even and odd  */
    xq = xp = x;
    yp = y;
    for (i = mv2; --i >= 0;) {
        *xp++ = *xq++;
        *yp++ = *xq++;
    }
    
    if (fft(x, y, mv2) == -1)    /* m / 2 point fft */
        return (-1);
    
    
    /***********************
     * SIN table generation *
     ***********************/
    
    if ((_sintbl == 0) || (maxfftsize < m)) {
        tblsize = m - m / 4 + 1;
        arg = PI / m * 2;
        if (_sintbl != 0)
            free(_sintbl);
        _sintbl = sinp = dgetmem(tblsize);
        *sinp++ = 0;
        for (j = 1; j < tblsize; j++)
            *sinp++ = sin(arg * (double) j);
        _sintbl[m / 2] = 0;
        maxfftsize = m;
    }
    
    n = maxfftsize / m;
    sinp = _sintbl;
    cosp = _sintbl + maxfftsize / 4;
    
    xp = x;
    yp = y;
    xq = xp + m;
    yq = yp + m;
    *(xp + mv2) = *xp - *yp;
    *xp = *xp + *yp;
    *(yp + mv2) = *yp = 0;
    
    for (i = mv2, j = mv2 - 2; --i; j -= 2) {
        ++xp;
        ++yp;
        sinp += n;
        cosp += n;
        yt = *yp + *(yp + j);
        xt = *xp - *(xp + j);
        *(--xq) = (*xp + *(xp + j) + *cosp * yt - *sinp * xt) * 0.5;
        *(--yq) = (*(yp + j) - *yp + *sinp * yt + *cosp * xt) * 0.5;
    }
    
    xp = x + 1;
    yp = y + 1;
    xq = x + m;
    yq = y + m;
    
    for (i = mv2; --i;) {
        *xp++ = *(--xq);
        *yp++ = -(*(--yq));
    }
    
    return (0);
}

double *_sintbl = 0;
int maxfftsize = 0;

static int checkm(const int m)
{
    int k;
    
    for (k = 4; k <= m; k <<= 1) {
        if (k == m)
            return (0);
    }
    fprintf(stderr, "fft : m must be a integer of power of 2!\n");
    
    return (-1);
}

int fft(double *x, double *y, const int m)
{
    int j, lmx, li;
    double *xp, *yp;
    double *sinp, *cosp;
    int lf, lix, tblsize;
    int mv2, mm1;
    double t1, t2;
    double arg;
    int checkm(const int);
    
    /**************
     * RADIX-2 FFT *
     **************/
    
    if (checkm(m))
        return (-1);
    
    /***********************
     * SIN table generation *
     ***********************/
    
    if ((_sintbl == 0) || (maxfftsize < m)) {
        tblsize = m - m / 4 + 1;
        arg = PI / m * 2;
        if (_sintbl != 0)
            free(_sintbl);
        _sintbl = sinp = dgetmem(tblsize);
        *sinp++ = 0;
        for (j = 1; j < tblsize; j++)
            *sinp++ = sin(arg * (double) j);
        _sintbl[m / 2] = 0;
        maxfftsize = m;
    }
    
    lf = maxfftsize / m;
    lmx = m;
    
    for (;;) {
        lix = lmx;
        lmx /= 2;
        if (lmx <= 1)
            break;
        sinp = _sintbl;
        cosp = _sintbl + maxfftsize / 4;
        for (j = 0; j < lmx; j++) {
            xp = &x[j];
            yp = &y[j];
            for (li = lix; li <= m; li += lix) {
                t1 = *(xp) - *(xp + lmx);
                t2 = *(yp) - *(yp + lmx);
                *(xp) += *(xp + lmx);
                *(yp) += *(yp + lmx);
                *(xp + lmx) = *cosp * t1 + *sinp * t2;
                *(yp + lmx) = *cosp * t2 - *sinp * t1;
                xp += lix;
                yp += lix;
            }
            sinp += lf;
            cosp += lf;
        }
        lf += lf;
    }
    
    xp = x;
    yp = y;
    for (li = m / 2; li--; xp += 2, yp += 2) {
        t1 = *(xp) - *(xp + 1);
        t2 = *(yp) - *(yp + 1);
        *(xp) += *(xp + 1);
        *(yp) += *(yp + 1);
        *(xp + 1) = t1;
        *(yp + 1) = t2;
    }
    
    /***************
     * bit reversal *
     ***************/
    j = 0;
    xp = x;
    yp = y;
    mv2 = m / 2;
    mm1 = m - 1;
    for (lmx = 0; lmx < mm1; lmx++) {
        if ((li = lmx - j) < 0) {
            t1 = *(xp);
            t2 = *(yp);
            *(xp) = *(xp + li);
            *(yp) = *(yp + li);
            *(xp + li) = t1;
            *(yp + li) = t2;
        }
        li = mv2;
        while (li <= j) {
            j -= li;
            li /= 2;
        }
        j += li;
        xp = x + j;
        yp = y + j;
    }
    
    return (0);
}

//extern "C" fillz(void *ptr, const size_t size, const int nitem);

void fillz(void *ptr, const size_t size, const int nitem)
{
    long n;
    //MIP
    //char *p = (char*)malloc(sizeof(char));
    char *p = ptr;
    
    n = size * nitem;
    while (n--)
        *p++ = '\0';
}


void freqt(double *c1, const int m1, double *c2, const int m2, const double a)
{
    int i, j;
    double b;
    static double *d = NULL, *g;
    static int size;
    
    if (d == NULL) {
        size = m2;
        d = dgetmem(size + size + 2);
        g = d + size + 1;
    }
    
    if (m2 > size) {
        free(d);
        size = m2;
        d = dgetmem(size + size + 2);
        g = d + size + 1;
    }
    
    b = 1 - a * a;
    fillz(g, sizeof(*g), m2 + 1);
    
    for (i = -m1; i <= 0; i++) {
        if (0 <= m2)
            g[0] = c1[-i] + a * (d[0] = g[0]);
        if (1 <= m2)
            g[1] = b * d[0] + a * (d[1] = g[1]);
        for (j = 2; j <= m2; j++)
            g[j] = d[j - 1] + a * ((d[j] = g[j]) - g[j - 1]);
    }
    
    movem(g, c2, sizeof(*g), m2 + 1);
    
    return;
}

//SPTK end

