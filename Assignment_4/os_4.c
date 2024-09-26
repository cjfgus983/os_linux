#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define MAX_LENGTH 5000

// ---------------------------------------------------
long long int virtual_address[5000]; // 가상주소
int no[5000]; // 그냥 인덱스

int page_no[5000]; //
//int page_table[5000]; // 페이지 테이블 -> page_table[page_no] == -1 이면 해당 page_no는 안올라가 있음 
//프레임에 올라가있는 페이지들
int frame_list[200]; // (메인메모리)프레임의 개수가 8개라면 그 안에 들어가는 page_no 

int frame_no[MAX_LENGTH]; // 가상주소가 연결될 프레임
long long int physical_address[5000]; // 실제 주소
char page_fault[5000]; // 'H' or 'F' 
int pageFault_count = 0; // 전체 발생한 페이지 폴트 횟수

int input_vaLength;
int input_pageSize;
int input_memSize;

int frameNum; // 프레임 개수

long long int vaLength;
long long int pageSize;
long long int memSize;
int algo;
int fileForm;

//들어온지 오래된 얘 내보내기 
void FIFO()
{
    //page No 저장
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        page_no[i] = virtual_address[i] / pageSize;
    }

    //PageFault 저장 , 프레임 할당
    int index = 0;
    for (int i = 0; i < MAX_LENGTH; i++) // 모든 가상명령어에 대해 일단 15 == 5000
    {
        int find = 0;
        for (int k = 0; k < frameNum; k++)//사용중인 프레임에 내가 있는지(page_no가 있는지 확인 )
        {
            if (frame_list[k] == page_no[i])//만약 있으면
            {
                find = 1;
                frame_no[i] = k;
                page_fault[i] = 'H';
                break;
            }
        }
        if (find == 0)//못찾았으면
        {
            page_fault[i] = 'F';
            pageFault_count++;
            //쫓아낼 애를 찾아야함
            frame_no[i] = index;
            frame_list[index] = page_no[i];
            index = (index + 1) % frameNum;
        }

        physical_address[i] = (frame_no[i] * pageSize) + (virtual_address[i] % pageSize);
    }
}

//만약 끝까지 사용 안되는 애가 2 이상이면 가장 먼저 들어온 애 내보내기
void Optimal()
{
    int when[100]; // 언제 교체 되었는지 별도 저장
    //page No 저장
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        page_no[i] = virtual_address[i] / pageSize;
    }

    //PageFault 저장 , 프레임 할당
    int index = 0;
    for (int i = 0; i < MAX_LENGTH; i++) // 모든 가상명령어에 대해
    {
        int find = 0;
        for (int k = 0; k < frameNum; k++) // 사용중인 프레임에 내가 있는지(page_no가 있는지 확인)
        {
            if (frame_list[k] == page_no[i]) // 만약 있으면
            {   
                find = 1;
                frame_no[i] = k;
                page_fault[i] = 'H';
                break;
            }
        }
        if (find == 0)//못찾았으면
        {
            page_fault[i] = 'F';
            pageFault_count++;
            //쫓아낼 애를 찾아야함
            int victimIndex = -1;
            int farthestFuture = -1; //

            int empty = 0;
            for (int k = 0; k < frameNum; k++) // 프레임 리스트 안을 훑어
            {
                if (frame_list[k] == -1) // 만약 프레임 리스트에서 -1를 발견하면 해당 프레임에 들어간다.
                {
                    victimIndex = k;
                    empty = 1; //비어있는 공간 있어요
                    break;
                }
            }

            if (empty == 0) // 비어있는 공간 없으면
            {
                int compare[100]; // 우선순위가 같은 framelist에서 누굴 victim 할지
                for (int i = 0; i < frameNum; i++)
                {
                    compare[i] = -1;
                }

                int cmpidx = 0;

                for (int k = 0; k < frameNum; k++) // 프레임 리스트 안을 훑어
                {
                    int future = 9999;
                    for (int j = i + 1; j < MAX_LENGTH; j++)
                    {
                        if (frame_list[k] == page_no[j]) // k는 page no
                        {
                            future = j; //프레임리스트 k번째 page는 j인덱스에서 또 쓰임
                            break;
                        }
                    }

                    if (future == 9999) //future 이 갱신 안됐다면
                    {
                        compare[cmpidx] = k;  // k번째 인덱스의 페이지는 뒤에 안나옴
                        cmpidx++;
                    }

                    if (future > farthestFuture) //가장 먼 인덱스 갱신
                    {
                        farthestFuture = future;
                        victimIndex = k;
                    }
                }
                if (cmpidx > 1) //뒤에 더이상 안나오는 페이지가 2개 이상이라면
                {
                    int v = compare[0]; //compare에는 인덱스가 들어있음
                    for (int q = 1; q < cmpidx; q++) //언제 frame list에 들어왔는지 비교 가장 먼저 들어온놈이 victim
                    {
                        if (when[compare[q]] < when[v]) //더 빨리 들어온 놈이 있으면 갱신
                        {
                            v = compare[q];
                        }
                    }
                    victimIndex = v;
                }
            }


            frame_no[i] = victimIndex;
            frame_list[victimIndex] = page_no[i];
            when[victimIndex] = i; // frame 리스트에 페이지가 없어서 i번째에 새롭게 갱신되는거니까 
        }

        physical_address[i] = (frame_no[i] * pageSize) + (virtual_address[i] % pageSize);
    }
}

//가장 마지막에 사용됐던 애 내보내기
void LRU()
{
    int used_time[100]; // 해당 페이지가 최근 사용된 위치
    // page No 저장
    for (int i = 0; i < MAX_LENGTH; i++) {
        page_no[i] = virtual_address[i] / pageSize;
    }

    for (int i = 0; i < 100; i++)
        used_time[i] = -1;

    for (int i = 0; i < MAX_LENGTH; i++) {
        int find = 0;
        for (int k = 0; k < frameNum; k++) {
            if (frame_list[k] == page_no[i]) { //찾았다면
                find = 1;
                frame_no[i] = k;
                page_fault[i] = 'H';
                used_time[frame_no[i]] = i; //해당 프레임 리스트는 i 인덱스에서 사용됨
                break;
            }
        }
        if (find == 0) // 못찾았으면 
        {
            page_fault[i] = 'F';
            pageFault_count++;

            int victimIndex = 0;
            int oldest_time = used_time[0];

            for (int k = 1; k < frameNum; k++) {
                if (used_time[k] < oldest_time) {
                    oldest_time = used_time[k];
                    victimIndex = k;
                }
            }

            frame_no[i] = victimIndex;
            frame_list[victimIndex] = page_no[i];
            used_time[victimIndex] = i;
        }

        physical_address[i] = (frame_no[i] * pageSize) + (virtual_address[i] % pageSize);
    }
}

void SecondChance() {
    // page No 저장
    for (int i = 0; i < MAX_LENGTH; i++) {
        page_no[i] = virtual_address[i] / pageSize;
    }

    // 모든 프레임 리스트에 대해
    int reference_bit[200];
    for (int i = 0; i < frameNum; i++) //일단 초기화
        reference_bit[i] = 0; // 0이면 victim으로 바꿔버리고 1이면 0으로 바꾸고 한번 봐줌

    int index = 0; // 프레임리스트를 도는 인덱스

    for (int i = 0; i < MAX_LENGTH; i++) { //모든 v.a를 돌면서
        int find = 0; 
        for (int k = 0; k < frameNum; k++)//사용중인 프레임에 내가 있는지(page_no가 있는지 확인 )
        {
            if (frame_list[k] == page_no[i])//만약 있으면
            {
                find = 1;
                frame_no[i] = k;
                page_fault[i] = 'H';
                reference_bit[k] = 1; // 해당 프레임은 참조 되었으므로 1로 변경
                break;
            }
        }

        if (find == 0) //못찾았다면
        {
            page_fault[i] = 'F';
            pageFault_count++;
            while (1) //reference bit 가 1이면 반복  // index를 돌려가며 reference bit가 1이면 봐주고 0이면 그 자리 차지
            {
                if (reference_bit[index] == 1) //기회가 한번 있는 경우
                {
                    reference_bit[index] = 0; // 기회 한 번 깍음
                    index = (index + 1) % frameNum;
                }
                else if (reference_bit[index] == 0) // 이 자리 차지하는 경우
                {
                    frame_list[index] = page_no[i];
                    frame_no[i] = index;
                    reference_bit[index] = 0; // write는 참조 아님
                    index = (index + 1) % frameNum;
                    break;
                }
            }
        }

        physical_address[i] = (frame_no[i] * pageSize) + (virtual_address[i] % pageSize);
    }
}

int main() {

    printf("\nA. Simulation에 사용할 가상주소 길이를 선택하시오 (1. 18bits  2.19bits  3. 20bits): ");
    scanf("%d", &input_vaLength);
    if (input_vaLength == 1)
    {
        vaLength = 262144;
    }
    else if (input_vaLength == 2)
    {
        vaLength = 524288;
    }
    else if (input_vaLength == 3)
    {
        vaLength = 1048576;
    }
    else
    {
        printf("Error!!\n");
        return 1;
    }

    printf("\nB. Simulation에 사용할 페이지(프레임)의 크기를 선택하시오 (1. 1KB  2. 2KB  3. 4KB): ");
    scanf("%d", &input_pageSize);
    if (input_pageSize == 1)
    {
        pageSize = 1024;
    }
    else if (input_pageSize == 2)
    {
        pageSize = 1024 * 2;
    }
    else if (input_pageSize == 3)
    {
        pageSize = 1024 * 4;
    }
    else
    {
        printf("Error!!\n");
        return 1;
    }

    printf("\nC. Simulation에 사용할 물리메모리의 크기를 선택하시오 (1. 32KB  2. 64KB ): ");
    scanf("%d", &input_memSize);
    if (input_memSize == 1)
    {
        memSize = 1024 * 32;
    }
    else if (input_memSize == 2)
    {
        memSize = 1024 * 64;
    }
    else
    {
        printf("Error!!\n");
        return 1;
    }
    frameNum = memSize / pageSize; // 프레임 테이블 개수

    printf("\nD. Simulation에 적용할 Page Replacement 알고리즘을 선택하시오\n");
    printf("(1. Optimal  2. FiFO  3. LRU  4.Second-Chance): ");
    scanf("%d", &algo);

    printf("\nE. 가상주소 스트링 입력방식을 선택하시오\n");
    printf("(1. input.in 자동 생성  2. 기존 파일 사용): ");
    scanf("%d", &fileForm);

    const char* filename = "";
    char* dfilename = NULL;
    char buffer[100]; // 예시: 최대 100자까지 파일 이름 입력 받음
    if (fileForm == 1)
    {
        //파일 자동생성
        if (fileForm == 1)
        {
            int num_numbers = 5000; // 생성할 난수의 개수
            filename = "input.in"; // 저장할 파일명

            srand(time(NULL));
            FILE* file = fopen(filename, "w");
            if (file == NULL) {
                printf("파일을 열 수 없습니다.\n");
                return 1;
            }
            for (int i = 0; i < num_numbers; i++) {
                long long random_number = (long long)rand() * vaLength / RAND_MAX; // 랜덤한 정수 생성
                fprintf(file, "%lld\n", random_number); // 파일에 쓰기
                //printf("%d\n", random_number);
            }
            fclose(file);
        }
    }
    else if (fileForm == 2)
    {
        printf("\nF. 입력 파일 이름을 입력하시오: \n");
        scanf("%s", buffer);
        dfilename = (char*)malloc(strlen(buffer) + 1);
        strcpy(dfilename, buffer);
    }
    else
    {
        printf("error!\n");
        return 1;
    }
    //초기화================================================
    for (int i = 0; i < MAX_LENGTH; i++) // 인덱스 초기화
    {
        no[i] = i + 1;
    }
    for (int i = 0; i < MAX_LENGTH; i++) // 프레임 리스트 할당 번호
    {
        frame_no[i] = -1;
    }
    for (int i = 0; i < MAX_LENGTH; i++) // 
    {
        physical_address[i] = -1;
    }

    for (int i = 0; i < 200; i++) // 프레임 리스트 초기화 fifo 에서 큐 역할
    {
        frame_list[i] = -1;
    }
    const char* input_file = "";
    if (fileForm == 1)
    {
        input_file = filename;
    }
    else if (fileForm == 2)
    {
        input_file = dfilename;
    }

    FILE* file = fopen(input_file, "r");
    if (file == NULL) {
        printf("파일 불러오기 실패\n");
        return 1;
    }
    //저장하고 출력해보자
    for (int i = 0; i < MAX_LENGTH; i++) {
        if (fscanf(file, "%lld", &virtual_address[i]) != 1) {
            printf("Error reading virtual addresses from the input file.\n");
            return 1;
        }
        if (virtual_address[i] > vaLength)
        {
            printf("설정한 길이보다 긴 가상주소가 있음\n");
            return 1;
        }
    }
    fclose(file);
    
    //결과 파일 저장
    char output_filename[100];

    if (algo == 1)
    {
        Optimal();
        strcpy(output_filename, "output.opt");
    }
    else if (algo == 2)
    {
        FIFO();
        strcpy(output_filename, "output.fifo");
    }
    else if (algo == 3)
    {
        LRU();
        strcpy(output_filename, "output.lru");
    }
    else if (algo == 4)
    {
        SecondChance();
        strcpy(output_filename, "output.sc");
    }
    else
    {
        printf("error\n");
        return 1;
    }

    FILE* output_file = fopen(output_filename, "w"); // 파일 열기
    if (output_file == NULL) {
        printf("파일을 열 수 없습니다.\n");
        return 1;
    }
    fprintf(output_file, "No.\tV.A.\tPage No.\tFrame No.\tP.A.\tPage Fault\n");
    for (int i = 0; i < MAX_LENGTH; i++) {
        fprintf(output_file, "%d\t%lld\t%d\t\t%d\t\t%lld\t%c\n", no[i], virtual_address[i], page_no[i], frame_no[i], physical_address[i], page_fault[i]);
    }
    fprintf(output_file, "Total Number of Page Faults: %d\n", pageFault_count);

    fclose(output_file);

    /*printf("No.          V.A.          Page No.          Frame No.          P.A.          Page Fault\n");
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        printf("%-12d %-13lld %-17d %-18d %-13lld %c\n", no[i], virtual_address[i], page_no[i], frame_no[i], physical_address[i], page_fault[i]);
    }
    printf("Total Number of Page Faults: %d\n", pageFault_count);*/
}   
