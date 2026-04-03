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
    (ta) / (tb) * (tb); \
})

#define abs(a) ({ \
    register __typeof__(a) ta = (a); ta < 0 ? -ta : ta; \
})

#define cmp(a, b) ({ \
    register __typeof__(a) ta = (a); \
    register __typeof__(b) tb = (b); \
    ta < tb ? -1 : (ta == tb ? 0 : 1); \
})

#endif