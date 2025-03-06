#ifndef __DEBUG_KALLSYMS_H__
#define __DEBUG_KALLSYMS_H__

extern unsigned long dbg_kallsyms_addr[] __attribute__((weak));
extern long dbg_kallsyms_syms_num __attribute__((weak));
extern long dbg_kallsyms_idx[] __attribute__((weak));
extern char* dbg_kallsyms_names __attribute__((weak));

#endif