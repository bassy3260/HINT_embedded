#include <stdio.h>

int main(void)
{
    int num1 = 10;
    printf("더하기전에 num1 = %d\n", num1); // 처음 값
    printf("출력해본 num1++ = %d\n", num1++); // 아직 안더해짐

    // 이 구간부터 더해져있음

    printf("출력 후 num1 = %d\n", num1); // 출력해보면 더해져있음.
    
    int num2 = 10;
     
    printf("더하기전에 num2 = %d\n", num2); // 처음 값
    printf("출력해본 ++num2 = %d\n", ++num2); //여기부터 더해져있음
    printf("출력 후 num2 = %d\n", num2); 
    return 0;
}