#ifndef __LIB_ALGORITHM_H__
#define __LIB_ALGORITHM_H__

#define max(a, b) ({ \
    __typeof__(a) ta = (a); \
    __typeof__(b) tb = (b); \
    ta < tb ? tb : ta; \
})

#define min(a, b) ({ \
    __typeof__(a) ta = (a); \
    __typeof__(b) tb = (b); \
    ta > tb ? tb : ta; \
})

#define upAlign(a, b) ({ \
    __typeof__(a) ta = (a); \
    __typeof__(b) tb = (b); \
    (ta + tb - 1) / (tb) * (tb); \
})

#define downAlign(a, b) ({ \
    __typeof__(a) ta = (a); \
    __typeof__(b) tb = (b); \
    (ta) / (tb); \
})

#endif