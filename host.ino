#include <Adafruit_NeoPixel.h>

#include <Wire.h>
#include <Keypad.h>

#define MEU_ENDERECO 1
#define OUTRO_ENDERECO 2

#define LEDSTRIP_TICTACTOE  12
#define LEDS_COUNT  9
Adafruit_NeoPixel strip(LEDS_COUNT, LEDSTRIP_TICTACTOE, NEO_GRB + NEO_KHZ800);

const byte ROWS = 3;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {11,21,31}, // xy, ex: 12 vira x = 1 e y = 2
  {12,22,32}, // ele vira o char q tem esse valor, ai fica facil de pega as coordenada
  {13,23,33}
};

byte rowPins[ROWS] = {11,10,9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7,6,5}; //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS); 

enum ESTADO_PONTO{VAZIO, CIRCULO, X};

enum {PLAYER1, PLAYER2, WAITING} estado_jogo = WAITING;

enum ESTADO_PONTO player1 = X;
enum ESTADO_PONTO player2 = CIRCULO; // por padrao o arduino host vai ser o X e o client vai ser o circulo, mas qlqr coisa da pra mudar
enum ESTADO_PONTO adicionar = X; // criminoso eu nao conseguir passar enum como parametro na funcao no tinker cad

enum ESTADO_PONTO pontos[3][3] = {
  {VAZIO, VAZIO, VAZIO},
  {VAZIO, VAZIO, VAZIO},
  {VAZIO, VAZIO, VAZIO}
};

int n_jogadas = 0; // numero de jogadas

void setup()
{
  Wire.begin(MEU_ENDERECO);
  Wire.onReceive(receiveEvent);
  Serial.begin(9600);       
}

bool aguardando_mensagem;

void espera_mensagem()
{
  aguardando_mensagem = true;
  while (aguardando_mensagem) {}
}

void envia_mensagem(char pos) // a unica coisa q esse vai envia o sinal q o outro pode apertar / onde ligar o led
{
  //delay(random(300, 700));
  Wire.beginTransmission(OUTRO_ENDERECO); // transmit to device #1
  Wire.write(pos);             // sends one byte  
  Wire.endTransmission();    // stop transmitting
}

boolean start_game = false;
void new_game(){
  for(int i = 0; i < ROWS; i++){
    for(int z = 0; z < COLS; z++){
      pontos[i][z] = VAZIO;
    }
  }
  n_jogadas = 0;
  envia_mensagem('C');
  strip.clear();
  strip.show();
  start_game = false;
  Serial.println("Jogador aperte qualquer botao no teclado para decidir quem ira comecar");
  while(!start_game){
    int key = customKeypad.getKey();
    if(key != NO_KEY) {
      start_game = true;
      estado_jogo = PLAYER1;
      Serial.println("Player 1 vai primeiro");
    }
  }
}

void loop()
{
  if(estado_jogo == WAITING){
    new_game();
  }else if(estado_jogo == PLAYER1){
    keypad();
  }else{
    espera_mensagem();
  }
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  int msg = Wire.read();
  if(estado_jogo == PLAYER1) return;
  if(!start_game){
    start_game = true;
    Serial.println("Player 2 vai primeiro");
    adicionar = VAZIO;
    estado_jogo = PLAYER2;
    return;
  }
  adicionar = player2;
  add_ponto(msg);
  aguardando_mensagem = false;
  if(checkFim(msg)){
    adicionar = VAZIO;
    return;
  }
  Serial.println("Vez do player 1");
  estado_jogo = PLAYER1;

}

void keypad(){
  int key = customKeypad.getKey();
  if(key != NO_KEY){
    int x = getIntDigit(key, 0);
    int y = getIntDigit(key, 1);
    adicionar = player1;
    if(!add_ponto(x, y)) {
      Serial.println("Posicao ocupada");
      return;
    }
    envia_mensagem(key);
    if(checkFim(x, y)){
      adicionar = VAZIO;
      return;
    }
    adicionar = VAZIO;
    estado_jogo = PLAYER2;
    Serial.println("Vez do player 2!");
    //espera_mensagem();
  }
}

int getIntDigit(int n, int digit){
  int result = (String(n).charAt(digit)) - '0';
  return result;
}

int rgb_circulo[] = {255, 0, 0};
int rgb_x[] = {0, 0, 255};
boolean add_ponto(char xy){return add_ponto(getIntDigit(xy, 0), getIntDigit(xy, 1));}
boolean add_ponto(int x, int y){ // retorna false se nao deu pra colocar no lugar pq ja tem um
  int led = 0;
  y -= 1;
  x -= 1;
  if(pontos[x][y] != VAZIO) return false;
  pontos[x][y] = adicionar;
  switch(x){
    case 1: led += 3; break;
    case 2: led += 6; break;
  }
  led += y;
  if(adicionar == X){
    strip.setPixelColor(led, rgb_x[0], rgb_x[1], rgb_x[2]);
  }else if(adicionar == CIRCULO){
    strip.setPixelColor(led, rgb_circulo[0], rgb_circulo[1], rgb_circulo[2]);
  }
  n_jogadas++;
  strip.show();
  return true;
}

void winner(){
  String vencedor;
  if(adicionar == player1){
    Serial.print("Player 1 ");
  }else{
    Serial.print("Player 2 ");
  }
  Serial.println("e o vencedor!");
  delay(1000);
  start_game = false;
  estado_jogo = WAITING;
}

boolean checkFim(char xy){return checkFim(getIntDigit(xy, 0), getIntDigit(xy, 1));}
boolean checkFim(int novoX, int novoY){ // pra nao checar toda possibilidade possivel sempre pra ver se alguem ganhou/empatou, verifica so as linha e colunas da onde foi inserido
  novoX -= 1;
  novoY -= 1;
  //checa coluna
  for(int i = 0; i < ROWS; i++){
    if(pontos[novoX][i] != adicionar) break;
    if(i == ROWS - 1){
      // player que acabou de adicionar venceu
      winner();
      return true;
    }
  } 

  //checa linha
  for(int i = 0; i < COLS; i++){
    if(pontos[i][novoY] != adicionar) break;
    if(i == COLS - 1){
      // player que acabou de adicionar venceu
      winner();
      return true;
    }
  } 

  // checa diagonal se estiver
  if(novoX == novoY){
    for(int i = 0; i < COLS; i++){
      if(pontos[i][i] != adicionar) break;
      if(i == COLS - 1){
        // player que acabou de adicionar venceu
        winner();
        return true;
      }
    } 
  }
  // checa antidiagonal se estiver
  if(novoX + novoY == ROWS - 1){ // isso assume q rows vai sempre ser igual a cols, q no jogo da velha sempre vai ser 
    for(int i = 0; i < ROWS; i++){
      if(pontos[i][ROWS-1-i] != adicionar) break;
      if(i == ROWS - 1){
        // player que acabou de adicionar venceu
        winner();
        return true;
      }
    } 
  }

  if(n_jogadas == ROWS*COLS){
    // empate
    Serial.println("Empate!");
    start_game == false;
    delay(1000);
    estado_jogo = WAITING;
    return true;
  }
  return false;
}
