#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define NUM_PROGRAMMERS 3
#define SHARED_MEM_SIZE 128

// Структура для хранения информации о программе
typedef struct {
    int id;
    int is_correct;
} Program;

// Структура для разделяемой памяти
typedef struct {
    Program programs[NUM_PROGRAMMERS];
    sem_t semaphores[NUM_PROGRAMMERS];
} SharedMemory;

void check_program(Program *program) {
    // Задаем случайное значение для is_correct
    program->is_correct = rand() % 2;
}

void fix_program(Program *program) {
    // Исправляем программу
    program->is_correct = 1;
}

void programmer(int id, SharedMemory *shared_mem) {
    while (1) {
        // Спим, если мы не пишем программу или не проверяем чужую программу
        printf("Programmer %d is sleeping\n", id);
        sleep(rand() % 5);

        // Проверяем чужую программу
        int other_id = (id + 1) % NUM_PROGRAMMERS;
        sem_wait(&shared_mem->semaphores[other_id]);

        printf("Programmer %d is checking the program of programmer %d\n", id, other_id);
        check_program(&shared_mem->programs[other_id]);

        sem_post(&shared_mem->semaphores[other_id]);

        // Проверяем свою программу
        sem_wait(&shared_mem->semaphores[id]);

        printf("Programmer %d is writing program\n", id);
        check_program(&shared_mem->programs[id]);

        // Если своя программа неправильная, исправляем ее
        if (!shared_mem->programs[id].is_correct) {
            printf("Programmer %d is fixing his own program\n", id);
            fix_program(&shared_mem->programs[id]);
        }

        sem_post(&shared_mem->semaphores[id]);
    }
}

int main() {
    // Создаем разделяемую память
    int fd = shm_open("/shared_mem", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedMemory));
    SharedMemory *shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    // Инициализируем семафоры
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        sem_init(&shared_mem->semaphores[i], 1, 1);
    }

    // Запускаем программистов
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        int pid = fork();
        if (pid == 0) {
            programmer(i, shared_mem);
            exit(0);
        }
    }

    // Ждем завершения работы всех программистов
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        wait(NULL);
    }

    // Удаляем разделяемую память
    munmap(shared_mem, sizeof(SharedMemory));
    shm_unlink("/shared_mem");

    return 0;
}