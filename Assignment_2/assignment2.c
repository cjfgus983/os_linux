#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/kernel.h>
#include <sys/syscall.h>


void del_space(char s[], long size)
{
    char* str = (char*)malloc(size);
    int i, k = 0;

    for (i = 0; i < strlen(s); i++)
        if (s[i] != ' ') str[k++] = s[i];

    str[k] = '\0';
    strcpy(s, str);
}

int main()
{
    char isError = 0;
    while (1)
    {
        long result = 0; // 결과
        isError = 0;
        char input[999];

        printf("Input: ");
        fgets(input, sizeof(input), stdin);

        long size = strlen(input);
        del_space(input, size);//공백제거

        if (strcmp(input, "\n") == 0) {
            //printf("프로그램을 종료합니다.\n");
            break; // 빈 줄이면 루프를 빠져나가 프로그램 종료
        }
        int op_count = 0;//연산자 개수
        int zero_count = 0;//입력받은 0의 개수
        
        for (int i = 0; i < strlen(input) - 1; i++)
        {
            if (input[i] == '0')//0의 개수 카운팅
            {
                zero_count++;
            }
        }
        if (zero_count == strlen(input) - 1)
        {
            isError = 1;
        }

        for (int i = 0; i < strlen(input) - 1; i++)
        {
            if (input[i] < '0' || input[i]>'9')//입력에서 문자가 걸리면 
            {
                op_count++;
                if (input[i] != '+' && input[i] != '-')//+나 -가 아니면
                {
                    isError = 1;
                    break;
                }
            }
        }

        if (isError == 1)
        {
            printf("Wrong Input!\n");
            continue;
        }

        // 1.숫자 뒤집기
        if (op_count == 0)
        {
            input[strlen(input) - 1] = '\0';
            long length = strlen(input);

            long value = syscall(453, input, length);

            printf("Output: %s\n", input);
            continue;
        }
        //2. 더하기 뺴기
        else if (op_count == 1)
        {
            char s_num1[999];//피연산자 1
            char s_num2[999];//피연산자 2

            //연산자 위치 찾기
            int loc = -1;
            for (int i = 0; i < strlen(input) - 1; i++) //oper 위치 받아오기
            {
                if (input[i] == '+' || input[i] == '-')
                {
                    loc = i;
                    s_num1[i] = '\0';
                    break;
                }
                //oper이전 숫자들은 s_num1에 대입
                s_num1[i] = input[i];
            }
            //나머지 s_num2 대입
            int s_num2_index = 0;
            for (int i = loc + 1; i < strlen(input) - 1; i++)
            {
                s_num2[s_num2_index++] = input[i];
            }
            s_num2[s_num2_index] = '\0';

            //===============
            long num1 = atol(s_num1);
            long num2 = atol(s_num2);

            if (num1 == 0 || num2 == 0) //예외처리
            {
                isError = 1;
            }

            if (input[loc] == '+')
            {
                long value = 0;
                value = syscall(454, num1, num2, &result);//minus
            }
            else if (input[loc] == '-')
            {
                result = syscall(451, num1, num2); //plus
            }
            else
            {
                isError = 1;
            }
        }
        //3. 다른 경우 오류 처리
        else
        {
            isError = 1;
        }

        if (isError == 1)
        {
            printf("Wrong Input!\n");
        }
        else
        {
            printf("Output: %ld\n", result);
        }

    }//while 끝


    return 0;
}

