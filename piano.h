#include<avr/io.h> //ATmega128 register정의
#define F_CPU 16000000UL //아트메가 주파수랑 맞추기
#define __DELAY_BACKWARD_COMPATIBLE__
#define HALF_CYC 500000UL
#include<util/delay.h>
#include<stdint.h>
#include<stdio.h>
#include<avr/interrupt.h>
#include "lcd.h" //lcd 헤더파일

#define DO 262//계이름
#define RE 294
#define MI 330
#define FA 349
#define SO 392
#define RA 440
#define SI 494
#define DDO 523

#define DBL 1208
#define FUL 623 //96bpm(한 박자 = 625ms)로 했으나 LCD write, interrupt 등으로 인한 딜레이로 약간의 수치 조정
#define HAFQUT 467
#define HAF 311
#define QUT 154
#define SIZE 31
#define ON 1
#define OFF 0

void initialize();//포트출력모드셋팅
void stay();//대기모드
void listen();//듣기모드
void game();//게임모드
void lcd_mode_stay();//대기모드 lcd화면
void lcd_mode_listen();//재생모드 lcd화면

void start_set();//듣기/게임모드 준비 함수(듣기/게임모드 시작할 때 스피커로 4박자, lcd에 숫자 출력)
void beep(uint16_t fr); //on time을 받아서 소리를 출력하는 함수
void tone(uint16_t fr); //주파수를 받아서 소리를 출력하는 함수
void tone3(uint16_t fr, uint16_t td, uint16_t tp); //소리를 원하는 길이만큼 출력하는 함수
int get_note(int pb); //스위치에서 음 가져오는 함수

unsigned int melody[]={MI, RE, DO, RE, MI, DDO, DDO, RA, SO, MI, DO, RE, MI, SO, MI, RE, MI, RE, DO, RE, MI, DDO, DDO, RA, SO, RA, DO, RE, MI, RE, DO}; //음 배열
unsigned int mel_len[]={HAFQUT, QUT, HAFQUT, QUT, FUL, FUL, DBL, DBL, HAFQUT, QUT, HAFQUT, QUT, FUL, HAFQUT, QUT, DBL+FUL, HAFQUT, QUT, HAFQUT, QUT, FUL, FUL, DBL, DBL, HAFQUT, QUT, HAFQUT, QUT, FUL, FUL, DBL+FUL}; // 음 길이unsigned int pause[]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FUL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FUL}; //쉼
unsigned int game_len[]={HAFQUT, 0, QUT, 0, HAFQUT, 0, QUT, 0, FUL, 0, FUL, 0, DBL, 0, DBL, 0, HAFQUT, 0, QUT, 0, HAFQUT, 0, QUT, 0, FUL, 0, HAFQUT, 0, QUT, 0, DBL+FUL, FUL, HAFQUT, 0, QUT, 0, HAFQUT, 0, QUT, 0, FUL, 0, FUL, 0, DBL, 0, DBL, 0, HAFQUT, 0, QUT, 0, HAFQUT, 0, QUT, 0, FUL, 0, FUL, 0, DBL+FUL, FUL}; //음 길이 + 쉼
unsigned int switch_num[]={2, 1, 0, 1, 2, 7, 7, 5, 4, 2, 0, 1, 2, 4, 2, 1, 2, 1, 0, 1, 2, 7, 7, 5, 4, 5, 0, 1, 2, 1, 0};//스위치 번호 순서unsigned char lcd_mel[] =
{ 0xc4,0xc2,0xc0,0xc2,0xc4,0xce,0xce,0xca,0xC8,0xC4,0xc0,0xc2,0xc4,0xc8,0xc4,0xc2,0xc4,0xc2,0xc0,0xc2,0xc4,0xce,0xce,0xca,0xC8,0xCA,0xc0,0xc2,0xc4,0xc2,0xc0 }; //재생모드 lcd화면 해당 멜로디 표시
unsigned int note_freq[8] = {DO, RE, MI, FA, SO, RA, SI ,DDO};
unsigned char lcd_note[8] = {0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce};

typedef enum {mode_stay, mode_listen, mode_game} MODE;//새로운형 MODE정의
MODE mode = mode_stay;
volatile int count = 0; int len_idx = 0; int mel_idx = 0; //음 길이. 음 인텍스

void initialize(){//초기포트셋팅함수
    DDRA = 0xff;//LCD 데이터포트(0~7번 사용)
    DDRB = 0x00; //스위치 8개 입력
    PORTB = 0xff; //내부 풀업
    DDRC = (1<<PORTC0) | (PORTC1) | (PORTC2);//LCD 제어포트(0~2번사용)
    DDRD = (1<<PORTD0); //메트로놈 스피커가 있는 아두이노에 신호를 주는 포트
    DDRF = (1<<PORTF1); //음 스피커 출력
    DDRG = 0x00; //스위치: 1번(재생모드),2번(게임모드) 입력
    PORTG = (1<<PORTG1) | (1<<PORTG2); //내부 풀업

    TCCR0 |= (1<<CS02);
    TIMSK |= (1<<TOIE0);
    TCNT0 = 6;
}

void stay(){ //대기모드함수
    lcd_mode_stay();//대기모드lcd화면
    while(1){
        if((PING & (1<<PORTG1))==0x00){//재생모드스위치on(포트G의 0번)
        LCD_wBCommand(0x01);//LCD화면 클리어
        mode=mode_listen;//재생모드로 바꿔준다.
        break;
        }
        else if((PING & (1<<PORTG2))==0x00){//게임모드스위치on(포트G의 1번)
            LCD_wBCommand(0x01);//LCD화면 클리어
            mode=mode_game;//게임모드로 바꿔준다.
            break;
        }
    }
}

void lcd_mode_stay() {
    LCD_Init();//LCD화면 초기화

    LCD_wBCommand(0x80);//시작지점 첫째줄
    LCD_wString("Switch1:listen");//문자출력
    LCD_wBCommand(0xc0);//시작지점 둘째줄
    LCD_wString(＂Switch2:game＂);//문자출력
}

void listen(){
    start_set();//노래시작전 준비
    PORTD = 0x01;
    _delay_us(20);
    PORTD = 0x00;

    LCD_wBCommand(0x80);
    LCD_wString("C D E F G A B CC");// LCD화면 첫 줄에 계이름 출력해놓는다.
    for (int I = 0; I < SIZE; i++) { //melody 의 개수만큼 반복
        LCD_wBCommand(lcd_mel[i]);// 음에 해당하는 위치에 문자시작지점으로
        LCD_wString(＂O”);// 음의 위치에 O표시
        tone3(melody[i], mel_len[i], pause[i]); //소리 출력
        LCD_wBCommand(lcd_mel[i]);//음에해당하는 위치에 문자시작지점으로
        LCD_wString(＂ ＂);// 한 음의 소리가 끝나면 O표시를 다시 빈칸으로
    }
    LCD_wBCommand(0x01);//LCD화면 클리어
    mode = mode_stay;
}

void game() { //게임모드
    len_idx = 0; mel_idx = 0; TCNT0 = 6; count = 0;
    int flag = 0; int score=0; int pb = 0; int note; char num[5];//PINB 저장 변수

    start_set();
    PORTD = (1<<PORTD0); //아두이노에 신호 보내서 메트로놈 스피커 on
    _delay_us(20);
    PORTD &= ~(1<<PORTD0);
    LCD_wBCommand(0x80);/////////////////////////////////////////
    LCD_wString("C D E F G A B CC");////////////////////////////
    sei(); //전역 인터럽트 활성화

    while (len_idx < SIZE * 2) {
        if ((PINB & 0xff) != 0xff) { //스위치가 눌려있을 때
            if (flag == OFF) { //스위치가 OFF -> ON으로 바뀌면 flag를 ON (스위치 눌릴 때 최초 실행)
                flag = ON;
                pb = (PINB^0xff); //PINB 반전
                note = get_note(pb); //PINB로부터 어떤 음이 눌렸는지 계산
                LCD_wBCommand(lcd_note[note]);
                if (note == switch_num[mel_idx]) {
                    //타이머 인터럽트에 의해 음 길이 마다 다음 인덱스로 넘어간다. 눌린 음과 현재 음이 같으면 성공
                    LCD_wString("O");
                    score++;
                }
                else LCD_wString("X");
            }
            tone(note_freq[note] + 2*note); //소리 출력
        }
        else { //스위치가 눌려있지 않을 때
            if (flag == ON) {
                flag = OFF;
                LCD_wBCommand(lcd_note[note]);
                LCD_wString(" ");
            }
        }
    }
    cli(); //전역 인터럽트 비활성화
    //게임모드가 끝나면 LCD화면에 맞은 개수 표시
    LCD_wBCommand(0x01);
    LCD_Init();
    LCD_wBCommand(0x80);
    LCD_wString("SCORE: ");
    sprintf(num, "%d", score);
    LCD_wBCommand(0xc0);
    LCD_wString(num);
    _delay_ms(2000);
    LCD_wBCommand(0x01);
    mode = mode_stay;
}

void start_set(){
    LCD_Init();/////////////////////////////
    char num[5]; //숫자->문자열을 담을 변수
    for (int i = 4; i > 0; i--) { //메트로놈 4번 반복
        sprintf(num, "%d", i); //숫자 -> 문자열 함수
        LCD_wBCommand(0x80);//LCD화면 1행1열에
        LCD_wString(num);//4321숫자출력
        for (int j = 0; j <821; j++) beep(95); //메트로놈의 주기와 반복 횟수는 항상 같기 때문에 상수
        _delay_ms(469);
    }
}

int get_note(int pb) { //PORTB 스위치 값에서 음 가져오는 함수
    int note = 0;
    while (pb != 1) {
        pb /= 2; note++;
    }
    return note;
}

void beep(uint16_t on_time) { //on time을 받아서 소리를 출력하는 함수
    PORTF |= (1<<PORTF1); _delay_us(on_time);
    PORTF &= ~(1<<PORTF1); _delay_us(on_time);
}

void tone(uint16_t fr) { //주파수를 받아서 소리를 출력하는 함수
    uint16_t on_time = 1./fr*500000.; //진동수 -> 반주기(us) 변환
    PORTF |= (1<<PORTF1); _delay_us(on_time);
    PORTF &= ~(1<<PORTF1); _delay_us(on_time);
}

void tone3(uint16_t fr, uint16_t td, uint16_t tp) { //소리를 원하는 길이만큼 출력하는 함수
    uint16_t on_time = 1./fr*500000.; //진동수 -> 반주기(us) 변환
    uint16_t n = td*1000./(2.*on_time); //duration time 동안 몇 번 반복할지 계산
    for (int i = 0; i < n; i++) beep(on_time);
    _delay_ms(tp);
}
