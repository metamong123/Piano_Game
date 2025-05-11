#include "piano.h"

ISR(TIMER0_OVF_vect) { //오버플로우 인터럽트 루틴
    TCNT0 = 6; //1ms
    count++;
    if (count > game_len[len_idx]) { //count가 음 길이(혹은 쉼 길이)보다 커지면
        len_idx++; //다음 인덱스로 이동
        mel_idx = (int)(len_idx / 2); //게임모드용 음 길이 인덱스는 쉼 길이와 합쳐져 있어서 인덱스가 2배
        count = 0; //카운트 초기화
    }
}

int main(){
    initialize();//포트 출력 모드 셋팅
    while(1){
        switch(mode){
            case mode_stay:
            stay(); break;//대기모드
            case mode_listen:
            listen(); break;//듣기모드
            case mode_game:
            game();//게임모드
        }
    }
}
