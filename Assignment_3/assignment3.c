#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>


struct sched_attr {
    uint32_t size;              
    uint32_t sched_policy;      // 스케줄러 정책
    uint64_t sched_flags;       
    int32_t  sched_nice;        // nice 값
    uint32_t sched_priority;    // fifo, RR 에서 우선순위

    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

static int sched_setattr(pid_t pid, const struct sched_attr* attr, unsigned int flags)
{
    return syscall(SYS_sched_setattr, pid, attr, flags);
}

// 시간을 시:분:초 형식으로 출력하는 함수 (소수점 아래 6자리까지)
void print_time(struct timeval time) {
    struct tm* tm_info;
    char time_str[20]; // "hh:mm:ss" 문자열을 저장할 버퍼

    tm_info = localtime(&time.tv_sec);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    printf("%s.%06ld", time_str, time.tv_usec);
}

void child_task()
{   
    int count = 0;
    int i, j, k;
    long A[100][100] = { 0, };
    long B[100][100] = { 0, };
    long result[100][100] = { 0, };

    while (count < 100)
    {
        for (k = 0; k < 100; k++) {
            for (i = 0; i < 100; i++) {
                for (j = 0; j < 100; j++) {
                    result[k][j] += A[k][i] * B[i][j];
                }
            }
        }
        count++;
    }
}

int main() {

    while (1)
    {
        printf("Input the Scheduling Polity to apply:\n");
        printf("1. CFS_DEFAULT\n");
        printf("2. CFS_NICE\n");
        printf("3. RT_FIFO\n");
        printf("4. RT_RR\n");
        printf("0. Exit\n");

        int input = 0;
        scanf("%d", &input);
        if (input == 1)
        {
            //============데이터 메세지 - 파이프===========
            int pipe_fd[2]; // 파이프 파일 디스크립터 배열
            int child_pid[21]; // 자식 프로세스의 PID를 저장할 배열

            if (pipe(pipe_fd) == -1) {  // 파이프 생성
                perror("파이프 생성 실패");
                return 1;
            }
            //===================================

            int cpu_core = 0; // 바인딩할 CPU 코어 번호 (0부터 시작)


        // 21개의 자식 프로세스 생성
            pid_t pid;
            int num_processes = 21;

            for (int i = 0; i < num_processes; i++) {
                pid = fork();

                //------------------생성된 순간 start에 넣어주어야 할듯 파이프로 넣어주자...
                struct timeval start_time;
                gettimeofday(&start_time, NULL);

                if (pid == -1) {
                    perror("fork");
                    return 1;
                }
                else if (pid == 0) {
                    close(pipe_fd[0]); // 자식은 읽기를 사용하지 않으므로 읽기 파이프 닫음

                    // 자식 프로세스는 특정 CPU 코어에 바인딩
                    cpu_set_t mask;
                    CPU_ZERO(&mask);
                    CPU_SET(cpu_core, &mask);

                    // 0 pid = 프로세스 자신을 mask = 0 0번 core에 할당
                    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
                        perror("sched_setaffinity");
                        return 1;
                    }

                    // 자식 프로세스의 작업 시작 시간 측정
                    struct timeval end_time;

                    // 자식 프로세스에서 원하는 작업 수행
                    child_task();

                    // 자식 프로세스의 작업 종료 시간 측정
                    gettimeofday(&end_time, NULL);

                    // 작업 경과 시간 계산
                    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

                    // 자식 프로세스의 PID와 NICE 값을 출력
                    printf("PID: %d | ", getpid());
                    int nice_val = getpriority(PRIO_PROCESS, 0);
                    printf("NICE: %d | ", nice_val);

                    // 시작 시간, 종료 시간 및 경과 시간 출력
                    printf("Start time: ");
                    print_time(start_time);
                    printf(" | ");

                    printf("End time: ");
                    print_time(end_time);
                    printf(" | ");

                    // 경과 시간을 초 단위로 출력
                    printf("Elapsed time: %.6lf \n", elapsed_time);

                    write(pipe_fd[1], &elapsed_time, sizeof(double)); // 파이프를 통해 데이터 전송
                    close(pipe_fd[1]); // 파이프 닫음
                    return 0;
                }
            }

            // ============================부모 프로세스==========
            close(pipe_fd[1]); // 부모는 쓰기를 사용하지 않으므로 쓰기 파이프 닫음

            //모든 자식 프로세스의 종료를 기다림
            for (int i = 0; i < num_processes; i++) {
                wait(NULL);
            }

            double sum;

            for (int i = 0; i < 21; i++) {
                double received_data;
                read(pipe_fd[0], &received_data, sizeof(double)); // 파이프에서 데이터 읽기
                sum = sum + received_data;
            }
            sum = sum / 21;

            printf("Schefuling Policy: CFS_DEFAULT | Average elapsed time: %.6lf\n", sum);

            return 0;
        }

        else if (input == 2)
        {
            //============데이터 메세지 - 파이프===========
            int pipe_fd[2]; // 파이프 파일 디스크립터 배열
            int child_pid[21]; // 자식 프로세스의 PID를 저장할 배열

            if (pipe(pipe_fd) == -1) {  // 파이프 생성
                perror("파이프 생성 실패");
                return 1;
            }
            //===================================

            int cpu_core = 0; // 바인딩할 CPU 코어 번호 (0부터 시작)


        // 21개의 자식 프로세스 생성
            pid_t pid;
            int num_processes = 21;

            for (int i = 0; i < num_processes; i++) {
                pid = fork();

                //------------------생성된 순간 start에 넣어주어야 할듯 파이프로 넣어주자...
                struct timeval start_time;
                gettimeofday(&start_time, NULL);

                if (pid == -1) {
                    perror("fork");
                    return 1;
                }
                else if (pid == 0) {
                    close(pipe_fd[0]); // 자식은 읽기를 사용하지 않으므로 읽기 파이프 닫음

                    // 자식 프로세스는 특정 CPU 코어에 바인딩
                    cpu_set_t mask;
                    CPU_ZERO(&mask);
                    CPU_SET(cpu_core, &mask);

                    // 0 pid = 프로세스 자신을 mask = 0 0번 core에 할당
                    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
                        perror("sched_setaffinity");
                        return 1;
                    }

                    //nice 값 다르게 지정
                    if (i < 7)
                    {
                        if (setpriority(PRIO_PROCESS, 0, 19) == -1) {
                            perror("setpriority");
                            return 1;
                        }
                    }
                    else if (13 < i && i < 21)
                    {
                        if (setpriority(PRIO_PROCESS, 0, -21) == -1) {
                            perror("setpriority");
                            return 1;
                        }
                    }

                    // 자식 프로세스의 작업 시작 시간 측정
                    struct timeval end_time;

                    // 자식 프로세스에서 원하는 작업 수행
                    child_task();

                    // 자식 프로세스의 작업 종료 시간 측정
                    gettimeofday(&end_time, NULL);

                    // 작업 경과 시간 계산
                    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

                    // 자식 프로세스의 PID와 NICE 값을 출력
                    printf("PID: %d | ", getpid());
                    int nice_val = getpriority(PRIO_PROCESS, 0);
                    printf("NICE: %d | ", nice_val);

                    // 시작 시간, 종료 시간 및 경과 시간 출력
                    printf("Start time: ");
                    print_time(start_time);
                    printf(" | ");

                    printf("End time: ");
                    print_time(end_time);
                    printf(" | ");

                    // 경과 시간을 초 단위로 출력
                    printf("Elapsed time: %.6lf \n", elapsed_time);

                    write(pipe_fd[1], &elapsed_time, sizeof(double)); // 파이프를 통해 데이터 전송
                    close(pipe_fd[1]); // 파이프 닫음
                    return 0;
                }
            }

            // ============================부모 프로세스==========
            close(pipe_fd[1]); // 부모는 쓰기를 사용하지 않으므로 쓰기 파이프 닫음

            //모든 자식 프로세스의 종료를 기다림
            for (int i = 0; i < num_processes; i++) {
                wait(NULL);
            }

            double sum;

            for (int i = 0; i < 21; i++) {
                double received_data;
                read(pipe_fd[0], &received_data, sizeof(double)); // 파이프에서 데이터 읽기
                sum = sum + received_data;
            }
            sum = sum / 21;

            printf("Schefuling Policy: CFS_NICE | Average elapsed time: %.6lf\n", sum);

            return 0;
        }

        else if (input == 3)
        {
            //============FIFO 전처리================
            struct sched_attr attr;
            // 초기화
            memset(&attr, 0, sizeof(attr));
            attr.size = sizeof(struct sched_attr);
            attr.sched_priority = 95; // 우선순위 값
            attr.sched_policy = SCHED_FIFO; // SCHED_FIFO는 상수로서 정의되어 있다.
            int result;
            //============데이터 메세지 - 파이프===========
            int pipe_fd[2]; // 파이프 파일 디스크립터 배열
            int child_pid[21]; // 자식 프로세스의 PID를 저장할 배열

            if (pipe(pipe_fd) == -1) {  // 파이프 생성
                perror("파이프 생성 실패");
                return 1;
            }
            //===================================

            int cpu_core = 0; // 바인딩할 CPU 코어 번호 (0부터 시작)


        // 21개의 자식 프로세스 생성
            pid_t pid;
            int num_processes = 21;

            for (int i = 0; i < num_processes; i++) {
                pid = fork();

                //------------------생성된 순간 start에 넣어주어야 할듯 
                struct timeval start_time;
                gettimeofday(&start_time, NULL);
                if (pid == -1) {
                    perror("fork");
                    return 1;
                }

                else if (pid == 0) { //자식 프로세스라면

                    result = sched_setattr(getpid(), &attr, 0);
                    if (result == -1) {
                        perror("Error calling sched_setattr.");
                    }

                    close(pipe_fd[0]); // 자식은 읽기를 사용하지 않으므로 읽기 파이프 닫음

                    // 자식 프로세스는 특정 CPU 코어에 바인딩
                    cpu_set_t mask;
                    CPU_ZERO(&mask);
                    CPU_SET(cpu_core, &mask);

                    // 0 pid = 프로세스 자신을 mask = 0 0번 core에 할당
                    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
                        perror("sched_setaffinity");
                        return 1;
                    }

                    // 자식 프로세스의 작업 시작 시간 측정
                    struct timeval end_time;


                    // 자식 프로세스에서 원하는 작업 수행
                    child_task();

                    // 자식 프로세스의 작업 종료 시간 측정
                    gettimeofday(&end_time, NULL);

                    // 작업 경과 시간 계산
                    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

                    // 자식 프로세스의 PID와 NICE 값을 출력
                    printf("PID: %d | ", getpid());
                    int nice_val = getpriority(PRIO_PROCESS, 0);
                    //printf("NICE: %d | ", nice_val);

                    // 시작 시간, 종료 시간 및 경과 시간 출력
                    printf("Start time: ");
                    print_time(start_time);
                    printf(" | ");

                    printf("End time: ");
                    print_time(end_time);
                    printf(" | ");

                    // 경과 시간을 초 단위로 출력
                    printf("Elapsed time: %.6lf \n", elapsed_time);

                    write(pipe_fd[1], &elapsed_time, sizeof(double)); // 파이프를 통해 데이터 전송
                    close(pipe_fd[1]); // 파이프 닫음
                    return 0;
                }
            }

            // ============================부모 프로세스==========
            close(pipe_fd[1]); // 부모는 쓰기를 사용하지 않으므로 쓰기 파이프 닫음

            //모든 자식 프로세스의 종료를 기다림
            for (int i = 0; i < num_processes; i++) {
                wait(NULL);
            }

            double sum;

            for (int i = 0; i < 21; i++) {
                double received_data;
                read(pipe_fd[0], &received_data, sizeof(double)); // 파이프에서 데이터 읽기
                sum = sum + received_data;
            }
            sum = sum / 21;

            printf("Schefuling Policy: RT_FIFO | Average elapsed time: %.6lf\n", sum);

            return 0;
        }

        else if (input == 4)
        {
            int ms = 100;
            const char* command;
            while (1)
            {
                //=======================타임 퀀퉘====================================
                printf("Input Time QuanTum(10 / 100 / 1000): \n");
                scanf("%d", &ms);
                if (ms == 10)
                {
                    command = "echo 10 > /proc/sys/kernel/sched_rr_timeslice_ms";
                    break;
                }
                else if (ms == 100)
                {
                    command = "echo 100 > /proc/sys/kernel/sched_rr_timeslice_ms";
                    break;
                }
                else if (ms == 1000)
                {
                    command = "echo 1000 > /proc/sys/kernel/sched_rr_timeslice_ms";
                    break;
                }
                else
                {
                    continue;
                }
            }
            int cResult = system(command);
            if (cResult == -1) {
                perror("system 명령 실행 오류");
                return 1;
            }
            //============RR 전처리========================================
            struct sched_attr attr;
            // 초기화
            memset(&attr, 0, sizeof(attr));
            attr.size = sizeof(struct sched_attr);
            attr.sched_priority = 95; // 우선순위 값
            attr.sched_policy = SCHED_RR; 
            int result;
            //============데이터 메세지 - 파이프================================
            int pipe_fd[2]; // 파이프 파일 디스크립터 배열
            int child_pid[21]; // 자식 프로세스의 PID를 저장할 배열

            if (pipe(pipe_fd) == -1) {  // 파이프 생성
                perror("파이프 생성 실패");
                return 1;
            }
            //=====================================================

            int cpu_core = 0; // 바인딩할 CPU 코어 번호 (0부터 시작)


        // =====================21개의 자식 프로세스 생성===========================
            pid_t pid;
            int num_processes = 21;

            for (int i = 0; i < num_processes; i++) {
                pid = fork();

                //------------------생성된 순간 start에 넣어주어야 할듯
                struct timeval start_time;
                gettimeofday(&start_time, NULL);

                if (pid == -1) {
                    perror("fork");
                    return 1;
                }
                else if (pid == 0) {
                    
                    result = sched_setattr(getpid(), &attr, 0);
                    if (result == -1) {
                        perror("Error calling sched_setattr.");
                    }

                    close(pipe_fd[0]); // 자식은 읽기를 사용하지 않으므로 읽기 파이프 닫음

                    // 자식 프로세스는 특정 CPU 코어에 바인딩
                    cpu_set_t mask;
                    CPU_ZERO(&mask);
                    CPU_SET(cpu_core, &mask);

                    // 0 pid = 프로세스 자신을 mask = 0 0번 core에 할당
                    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
                        perror("sched_setaffinity");
                        return 1;
                    }

                    // 자식 프로세스의 작업 시작 시간 측정
                    struct timeval end_time;

                    // 자식 프로세스에서 원하는 작업 수행
                    child_task();

                    // 자식 프로세스의 작업 종료 시간 측정
                    gettimeofday(&end_time, NULL);

                    // 작업 경과 시간 계산
                    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

                    // 자식 프로세스의 PID와 NICE 값을 출력
                    printf("PID: %d | ", getpid());
                    int nice_val = getpriority(PRIO_PROCESS, 0);
                    //printf("NICE: %d | ", nice_val);

                    // 시작 시간, 종료 시간 및 경과 시간 출력
                    printf("Start time: ");
                    print_time(start_time);
                    printf(" | ");

                    printf("End time: ");
                    print_time(end_time);
                    printf(" | ");

                    // 경과 시간을 초 단위로 출력
                    printf("Elapsed time: %.6lf \n", elapsed_time);

                    write(pipe_fd[1], &elapsed_time, sizeof(double)); // 파이프를 통해 데이터 전송
                    close(pipe_fd[1]); // 파이프 닫음
                    return 0;
                }
            }

            // ============================부모 프로세스==========
            close(pipe_fd[1]); // 부모는 쓰기를 사용하지 않으므로 쓰기 파이프 닫음

            //모든 자식 프로세스의 종료를 기다림
            for (int i = 0; i < num_processes; i++) {
                wait(NULL);
            }

            double sum;

            for (int i = 0; i < 21; i++) {
                double received_data;
                read(pipe_fd[0], &received_data, sizeof(double)); // 파이프에서 데이터 읽기
                sum = sum + received_data;
            }
            sum = sum / 21;

            printf("Schefuling Policy: RT_RR | Time Quantum: %d | Average elapsed time: %.6lf\n", ms, sum);

            return 0;
        }

        else if (input == 0)
            return 0;
        else
            continue;
    }

}



