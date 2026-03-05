#ifndef __CPU_DEFINE_H__
#define __CPU_DEFINE_H__

#define cpu_Desc_state_Active   0x1
#define cpu_Desc_state_Trapped  0x2
#define cpu_Desc_state_Pause    0x3

#define cpu_Desc_flag_EnablePreempt 0x1

#define cpu_var(name) __cpu_percpu_##name
#define cpu_varOffset(name) ((u64)&cpu_var(name) - (u64)&cpu_symbol_percpu)

#define cpu_declarevar(type, name) \
    extern __typeof__(type) cpu_var(name)

#define cpu_definevar(type, name) \
    __used__ __attribute__((__section__(".data.percpu"))) __typeof__(type) cpu_var(name)

#endif