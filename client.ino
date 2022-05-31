#include <Adafruit_NeoPixel.h>

#include <Wire.h>
#include <Keypad.h>

#define MEU_ENDERECO 2
#define OUTRO_ENDERECO 1

#define LEDSTRIP_XADREZ  12
#define LEDS_COUNT  9
Adafruit_NeoPixel strip(LEDS_COUNT, LEDSTRIP_XADREZ, NEO_GRB + NEO_KHZ800);

const byte ROWS = 3;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {11,21,31}, // xy, ex: 12 vira x = 1 e y = 2
  {12,22,32}, // ele vira o char q tem esse valor, ai fica facil de pega as coordenada
  {13,23,33}
};

boolean thisPlayer = false;
boolean game = false;

byte rowPins[ROWS] = {11,10,9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7,6,5}; //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS); 

void setup()
{
  Wire.begin(MEU_ENDERECO);
  Wire.onReceive(receiveEvent);
  Serial.begin(9600);       
  gameStart();
}

bool aguardando_mensagem;

enum ESTADO_PONTO{VAZIO, CIRCULO, X};

enum ESTADO_PONTO pontos[3][3] = {
  {VAZIO, VAZIO, VAZIO},
  {VAZIO, VAZIO, VAZIO},
  {VAZIO, VAZIO, VAZIO}
};

void espera_mensagem()
{
  aguardando_mensagem = true;
  while (aguardando_mensagem) {}
}

void envia_mensagem(char msg)
{
  //delay(random(300, 700));
  Wire.beginTransmission(OUTRO_ENDERECO); // transmit to device #1
  Wire.write(msg);              // sends one byte  
  Wire.endTransmission();    // stop transmitting
}

void gameStart(){
  int key = customKeypad.getKey();
  if(key != NO_KEY){
    game = true;
    Serial.println("Vez do player 2!");
    envia_mensagem(key);
  }
}

void loop()
{
  if(game) keypad();
  else gameStart();
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  char msg = Wire.read();
  aguardando_mensagem = false;
  if(msg == 'C'){
    for(int i = 0; i < ROWS; i++){
      for(int z = 0; z < COLS; z++){
        pontos[i][z] = VAZIO;
      }
    }
    thisPlayer = false;
    strip.clear();
    strip.show();
    game = false;
    return;
  }
  game = true;
  int x = getIntDigit(msg, 0);
  int y = getIntDigit(msg, 1);
  ligarLed(x, y);
  pontos[x - 1][y - 1] = X;
  Serial.println("Vez do player 2!");
  //Serial.println(x);         // print the integer
}

void keypad(){
  int key = customKeypad.getKey();
  if(key != NO_KEY){
    thisPlayer = true;
    int x = getIntDigit(key, 0);
    int y = getIntDigit(key, 1);
    if(ligarLed(x, y)){
      pontos[x-1][y-1] = CIRCULO;
      Serial.println("Vez do player 1!");
      envia_mensagem(key);
      espera_mensagem();
    }else{
      Serial.println("Posicao ocupada");
    }
  }
}

int getIntDigit(int n, int digit){
  int result = (String(n).charAt(digit)) - '0';
  return result;
}

int rgb_circulo[] = {255, 0, 0};
int rgb_x[] = {0, 0, 255};
boolean ligarLed(char xy){return ligarLed(getIntDigit(xy, 0), getIntDigit(xy, 1));}
boolean ligarLed(int x, int y){
  int led = 0;
  y -= 1;
  x -= 1;
  if(pontos[x][y] != VAZIO) return false;
  switch(x){
    case 1: led += 3; break;
    case 2: led += 6; break;
  }
  led += y;
  if(!thisPlayer){
    strip.setPixelColor(led, rgb_x[0], rgb_x[1], rgb_x[2]);
  }else{
    strip.setPixelColor(led, rgb_circulo[0], rgb_circulo[1], rgb_circulo[2]);
    thisPlayer = false;
  }
  strip.show();
  return true;
}
