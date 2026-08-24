#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
    char name[50];     // 이름 저장
    int student_id;    // 학번
    int c_score;       // c언어 점수
    int math_score;    // 수학 점수
    int english_score; // 영어 점수


    printf("이름을 입력하세요: ");
    scanf( "%s", &name);
    printf("학번을 입력하세요: ");
    scanf( "%d", &student_id);
    printf("c 언어 점수를 입력하세요: ");
    scanf( "%d", &c_score);
    printf("수학 점수를 입력하세요: ");
    scanf( "%d", &math_score);
    printf("영어 점수를 입력하세요: ");
    scanf( "%d", &english_score);
    

    printf("\n================================\n");
    printf("          학생 성적표\n");
    printf("================================\n");

    // 이름, 학번 출력
    printf("이름 : %s\n", name);
    printf("학번 : %d\n", student_id);
    printf("--------------------------------\n");

    // c언어, 수학, 영어 출력
    printf("C언어 점수 : %d\n", c_score);
    printf("수학 점수 : %d\n", math_score);
    printf("영어 점수 : %d\n", english_score);

    printf("--------------------------------\n");

    // 총점 평균 출력
    int total = c_score + math_score + english_score;
    printf("총점: (c_score + math_score + english_score) = %d\n", total);

    double average = (double)total / 3;
    printf("평균: %1.2f\n", average);

    printf("================================\n");

    return 0;
}