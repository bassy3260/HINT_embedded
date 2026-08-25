#include <stdio.h>

// 주소를 받아서 원래 사탕 개수를 바꾸는 함수
void addCandy(int *candyAddress)
{
    // *를 붙이면 주소에 들어 있는 진짜 값을 뜻한다.
    *candyAddress = *candyAddress + 5;
}

int main(void)
{
    // candy는 숫자를 담는 상자
    int candy = 3;

    // candyPointer는 candy 상자의 주소를 담는 주소 쪽지
    int *candyPointer = &candy;

    // 여기는 int*가 되어야한다.
    // int candyPointer = &candy;
    // 상자의 주소
    printf("candy 상자의 주소: %p\n", &candy);
    // 상자가 적힌 쪽지의 주소
    //얘가 이중포인터가 되는거임 ㅇㅇ
    printf("candyPointer 상자의 주소: %p\n", &candyPointer);

    // 상자에 뭐가 들어있는지?
    printf("처음 사탕 개수: %d\n", candy);
    // 쪽지에 적힌 곳으로 가면 있는 상자에 뭐가 들어있는지?
    printf("주소 쪽지로 본 사탕 개수: %d\n", *candyPointer);

    // 주소 쪽지를 이용해 원래 candy 상자의 값을 바꾼다.
    // 주소를 따라가서 그 곳에 있는 값을 10으로 바꿔 ~~ 
    *candyPointer = 10;
    printf("포인터로 바꾼 뒤: %d\n", candy);

    // 함수에도 candy 상자의 주소를 알려준다.
    addCandy(&candy);
    printf("함수에서 5개를 더한 뒤: %d\n", candy);

    return 0;
}
