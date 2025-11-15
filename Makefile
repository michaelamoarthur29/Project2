shm_proc: shm_processes.c
	gcc shm_processes.c -D_SVID_SOURCE -pthread -std=c99 -lpthread  -o shm_proc
example: example.c
	gcc example.c -pthread -std=c99 -lpthread  -o example
ref_psdd: ref_psdd.c
	gcc ref_psdd.c -D_SVID_SOURCE -pthread -std=c99 -lpthread -o ref_psdd
