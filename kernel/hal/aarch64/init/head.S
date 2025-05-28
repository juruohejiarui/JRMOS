.global 

_start:
	# set stack pointer
	ldr x0, hal_init_stk
	mov sp, x0

	# call hal_init_init
	bl hal_init_init
	
	b .