#include "max6675.h" //INCLUSÃO DE BIBLIOTECA
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Modbusino.h>
/************************************************
   ARDUINO - COMUNICAÇÃO SERIAL (MODBUS)
   https://github.com/stephane/modbusino
**************************************************/

ModbusinoSlave modbusino_slave(1); /* Inicializa o ID do Dispositivo*/
uint16_t tab_reg[10]; /* Aloca o Número de Registradores*/

int resistor = 7;
int fan = 6;
int botaoAumenta = 5; // BOTAO DE AUMENTAR A SP
int botaoDiminui = 4;
int botaoONOFF = 2;
int lastAumenta = HIGH;
int lastDiminui = HIGH;
int lastONOFF = HIGH;
bool teto = false; // 0 EMBAIXO 1 EM CIMA
bool chao = false;
float histerese = 0.25;
bool ONOFF = false;
int ktcSO = 8; //PINO DIGITAL (SO)
int ktcCS = 9; //PINO DIGITAL (CS)
int ktcCLK = 10; //PINO DIGITAL (CLK / SCK)
float set_point = 25.0;

MAX6675 ktc(ktcCLK, ktcCS, ktcSO); //CRIA UMA INSTÂNCIA UTILIZANDO OS PINOS (CLK, CS, SO)
LiquidCrystal_I2C lcd(0x27,16,2);

void setup() {
  modbusino_slave.setup(9600); /* Definindo a taxa de transferencia em bauds */

  pinMode(fan, OUTPUT);
  pinMode(resistor, OUTPUT);
  pinMode(botaoAumenta, INPUT_PULLUP);
  pinMode(botaoDiminui, INPUT_PULLUP);
  pinMode(botaoONOFF, INPUT_PULLUP);
  lcd.begin();
  lcd.clear();

  delay(500); //INTERVALO DE 500 MILISSEGUNDOS
}

void setDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("SP: ");
  lcd.setCursor(3, 0);
  lcd.print(set_point);
  lcd.setCursor(0, 1);
  lcd.print("MV: ");
  lcd.setCursor(3, 1);
  lcd.print(ktc.readCelsius());
  lcd.setCursor(9, 1);
  lcd.print("H: ");
  lcd.setCursor(11,1);
  lcd.print(histerese); 
  lcd.setCursor(13, 0);

  if (ONOFF) {
    lcd.print("ON ");
  } else {
    lcd.print("OFF");
  }
}

void readBotaoAumenta() {
  int readingAumenta = digitalRead(botaoAumenta);
  if (readingAumenta != lastAumenta) {
    if (readingAumenta == LOW) {
      set_point++;
    }
    delay(50);
  }
  if (tab_reg[3] == 1) {
    set_point++;
    tab_reg[3] = 0;
  }

  lastAumenta = readingAumenta;
}

void readBotaoDiminui() {
  int readingDiminui = digitalRead(botaoDiminui);
  if (readingDiminui != lastDiminui) {
    if (readingDiminui == HIGH) {
      set_point--;
    }
    delay(50);
  }
  if (tab_reg[4] == 1) {
    set_point--;
    tab_reg[4] = 0;
  }

  lastDiminui = readingDiminui;
}

void readBotaoONOFF() {
  int readingONOFF = digitalRead(botaoONOFF);
  if (readingONOFF != lastONOFF) {
    if (readingONOFF == LOW) {
      ONOFF = !ONOFF;
    }
    delay(50);
  }
  lastONOFF = readingONOFF;
}

void loop() {
  /*Exemplo de como passar valores para os registradores
    tab_reg[0] = 600;
    
    Exemplo de como recuperar valores dos registradores
    Variavel = tab_reg[1];    
  */

  setDisplay();
  ONOFF = tab_reg[2];
  histerese = (float) (tab_reg[7] / 100.0);
  readBotaoAumenta();
  readBotaoDiminui();
  readBotaoONOFF();
  if (ONOFF) {
    if (ktc.readCelsius() >= (set_point + histerese)) {
      digitalWrite(fan, HIGH);
      digitalWrite(resistor, LOW);
    }
    else if (ktc.readCelsius() <= (set_point - histerese)) {
      digitalWrite(resistor, HIGH);
    }
    if (ktc.readCelsius() < (set_point + histerese)) {
      digitalWrite(fan, LOW);
    }
  } else {
    digitalWrite(fan, LOW);
    digitalWrite(resistor, LOW);
  }

  tab_reg[0] = set_point;
  tab_reg[1] = (int16_t) (ktc.readCelsius() * 100);
  tab_reg[2] = ONOFF;
  tab_reg[5] = digitalRead(resistor);
  tab_reg[6] = digitalRead(fan);
  tab_reg[7] = (int16_t) (histerese * 100);

  modbusino_slave.loop(tab_reg, 10); //Atualizando o Modbus
  delay(200); //INTERVALO DE 200 MILISSEGUNDOS
}