//계이름
#define DO 523
#define FA 698
#define SO 784

int freq[32] = {DO, DO, DO, DO, FA, FA, FA, FA, DO, DO, DO, DO, SO, SO, SO, SO, DO, DO, DO, DO, FA, FA, FA, FA, DO, DO, DO, SO, DO, DO, DO, DO};

void setup() {
    pinMode(8,OUTPUT); 메트로놈 스피커 출력 핀
    pinMode(12,INPUT); //아트메가 신호 입력 핀
}

void loop() {
    if (digitalRead(12)) { //신호가 들어오면 메트로놈 스피커 on
        for (int i = 0; i < 31; i++) {
            tone(8, freq[i], 156);
            delay(625);
        }
    }
}
