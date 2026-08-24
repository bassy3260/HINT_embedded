#include <stdio.h>

int main(void) {
    char name[20]; //배열, 문자열
    printf("\n[문자열 입력]\n");

    printf("이름을 입력하세요 : ");
    // 배열은 앞에 &를 붙이지 않아도 된다. 배열의 이름은 이미 주소값이기 때문이다.
    scanf_s("%99s", name, (unsigned)sizeof(name));
    // 문자열을 최대 99자까지 입력받도록 제한하고, 배열의 크기를 지정하여 버퍼 오버플로우를 방지한다.
    printf("입력하신 이름은 %s 입니다.\n", name);
    return 0;
}