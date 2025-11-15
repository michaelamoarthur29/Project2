/*
 * ref_psdd.c
 *
 * Usage:
 *   ./ref_psdd <parents_flag> <num_students>
 *
 * parents_flag:
 *   1  -> Only Dear Old Dad (no Mom)
 *   2  -> Dear Old Dad + Lovable Mom
 *
 * num_students:
 *   number of Poor Student child processes to create
 *
 * Example:
 *   ./ref_psdd 1 3   -> Dad + 3 students
 *   ./ref_psdd 2 10  -> Dad + Mom + 10 students
 *
 * Behavior:
 *   Implements the semantics described in the assignment using:
 *     - shared memory via mmap on a small file ("bankacct.bin")
 *     - a named POSIX semaphore as mutex ("/bank_mutex")
 *
 * Notes:
 *   - All processes loop forever until you kill the parent (or kill all processes).
 *   - Each process seeds rand() with getpid() to get distinct behavior. 

 Code developed by chat gpt
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SHM_FILENAME "bankacct.bin"
#define SEM_NAME "/bank_mutex"
#define SHM_SIZE sizeof(int)

static sem_t *mutex = NULL;
static int *BankAccount = NULL;
static int shm_fd = -1;

/* Helper: random int in [0, max] */
static int rnd(int max) {
    if (max <= 0) return 0;
    return rand() % (max + 1);
}

void cleanup_on_exit(void) {
    if (BankAccount) {
        munmap(BankAccount, SHM_SIZE);
        BankAccount = NULL;
    }
    if (shm_fd >= 0) {
        close(shm_fd);
        shm_fd = -1;
    }
    /* Attempt to unlink semaphore; ignore errors */
    sem_close(mutex);
    sem_unlink(SEM_NAME);
}

/* Dad process */
void dear_old_dad_loop(void) {
    int localBalance;
    for (;;) {
        sleep(rnd(5)); /* 0-5 seconds */
        printf("Dear Old Dad: Attempting to Check Balance\n");

        int r = rnd(1); /* 0 or 1 -> even/odd */
        if ((r % 2) == 0) {
            /* attempt deposit if last localBalance < 100 */
            sem_wait(mutex);
            localBalance = *BankAccount;
            if (localBalance < 100) {
                int amount = rnd(100); /* 0..100 */
                if ((amount % 2) == 0) {
                    localBalance += amount;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                } else {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
                *BankAccount = localBalance;
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
            sem_post(mutex);
        } else {
            /* just check balance */
            sem_wait(mutex);
            localBalance = *BankAccount;
            sem_post(mutex);
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
        fflush(stdout);
    }
}

/* Mom process */
void lovable_mom_loop(void) {
    int localBalance;
    for (;;) {
        sleep(rnd(10)); /* 0-10 seconds */
        printf("Loveable Mom: Attempting to Check Balance\n");

        sem_wait(mutex);
        localBalance = *BankAccount;
        if (localBalance <= 100) {
            int amount = rnd(125); /* 0..125 */
            localBalance += amount;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", amount, localBalance);
            *BankAccount = localBalance;
        } else {
            /* optional: Mom just checks */
            printf("Lovable Mom: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(mutex);
        fflush(stdout);
    }
}

/* Student process */
void poor_student_loop(int id) {
    int localBalance;
    for (;;) {
        sleep(rnd(5)); /* 0-5 seconds */
        printf("Poor Student: Attempting to Check Balance\n");

        int r = rnd(1); /* 0 or 1 */
        if ((r % 2) == 0) {
            /* attempt withdraw */
            sem_wait(mutex);
            localBalance = *BankAccount;
            int need = rnd(50); /* 0..50 */
            printf("Poor Student needs $%d\n", need);
            if (need <= localBalance) {
                localBalance -= need;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
            *BankAccount = localBalance;
            sem_post(mutex);
        } else {
            sem_wait(mutex);
            localBalance = *BankAccount;
            sem_post(mutex);
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
        }
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <parents_flag (1 or 2)> <#students>\n", argv[0]);
        fprintf(stderr, "  1 => Dear Old Dad only\n  2 => Dear Old Dad + Lovable Mom\n");
        exit(EXIT_FAILURE);
    }

    int parents_flag = atoi(argv[1]);
    int num_students = atoi(argv[2]);
    if (parents_flag < 1) parents_flag = 1;
    if (num_students < 0) num_students = 0;

    /* Seed RNG */
    srand((unsigned int)(time(NULL) ^ getpid()));

    /* Prepare shared file for memory mapped integer */
    shm_fd = open(SHM_FILENAME, O_RDWR | O_CREAT, 0600);
    if (shm_fd < 0) {
        perror("open shm file");
        exit(EXIT_FAILURE);
    }
    /* ensure file is SHM_SIZE bytes */
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    BankAccount = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (BankAccount == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    /* initialize bank account to 0 */
    *BankAccount = 0;

    /* Setup semaphore (named). Unlink first to avoid leftover from previous runs. */
    sem_unlink(SEM_NAME);
    mutex = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open");
        munmap(BankAccount, SHM_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    /* Ensure cleanup on exit (best-effort) */
    atexit(cleanup_on_exit);

    /* Fork requested processes:
     * - First fork num_students children (Poor Students)
     * - Then optionally fork Mom
     * - Parent process becomes Dad
     *
     * Note: each forked child will call the appropriate process loop and never return.
     */

    for (int i = 0; i < num_students; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork student");
            /* continue trying to create others */
        } else if (pid == 0) {
            /* child student */
            srand((unsigned int)(time(NULL) ^ getpid()));
            poor_student_loop(i);
            /* never returns */
            _exit(0);
        } else {
            /* parent continues to create others */
        }
    }

    if (parents_flag >= 2) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork mom");
        } else if (pid == 0) {
            /* mom process */
            srand((unsigned int)(time(NULL) ^ getpid()));
            lovable_mom_loop();
            _exit(0);
        }
    }

    /* Parent process acts as Dear Old Dad */
    dear_old_dad_loop();

    /* never reached */
    return 0;
}
