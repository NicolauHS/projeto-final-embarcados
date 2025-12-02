#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/Picopixel.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SELECT_BUTTON 38
#define TRIGGER_1 2
#define ECHO_1 13
#define TRIGGER_2 14
#define ECHO_2 25
#define BUTTON_1 35
#define LED_GREEN 33
#define LED_RED 32
#define LED_BLUE 15

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int FACE_X = 96;
const int FACE_Y = 40;
const int FACE_R = 20;
const int EYE_OFFSET_X = 7;
const int EYE_OFFSET_Y = 7;
const int EYE_R = 2;
const int MOUTH_R = 10;
int people_count = 0;
int people_max = 20;
bool onMenu = false;
bool onMainScreen = false;
float dist1;
float dist2;
bool sensorsInverted = false;

enum MenuScreen {
  MENU_MAIN,
  MENU_CONFIG,
  MENU_VAGAS,
  MENU_DISTANCIA,
  MENU_INVERTER
};

MenuScreen currentMenu = MENU_MAIN;
int menuSelection = 0;
bool lastButton1State = HIGH;
bool lastSelectButtonState = HIGH;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

enum State {
  IDLE,
  FIRST1,
  FIRST2
};
State state = IDLE;

enum Event {
  NONE,
  ENTER,
  LEAVE
};

Event event = NONE;

int min_distance = 10;

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  pinMode(SELECT_BUTTON, INPUT_PULLUP);
  pinMode(TRIGGER_1, OUTPUT);
  pinMode(ECHO_1, INPUT);
  pinMode(TRIGGER_2, OUTPUT);
  pinMode(ECHO_2, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUTTON_1, INPUT);
}

void getDistance() {
  // Atualiza distância dos dois sensores ultrassônicos
  digitalWrite(TRIGGER_1, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIGGER_1, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_1, LOW);
  dist1 = pulseIn(ECHO_1, HIGH);
  dist1 = dist1 / 58;
  
  digitalWrite(TRIGGER_2, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIGGER_2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_2, LOW);
  dist2 = pulseIn(ECHO_2, HIGH);
  dist2 = dist2 / 58;
}

void drawArc(int x, int y, int radius, int startAngle, int endAngle) {
  // Desenha um arco no display, para desenhar o sorriso :)
  for (int angle = startAngle; angle <= endAngle; angle++) {
    float rad = angle * 3.14159 / 180.0;
    int px = x + radius * cos(rad);
    int py = y + radius * sin(rad);
    display.drawPixel(px, py, SSD1306_WHITE);
  }
}

void drawFace(int type=0) {
  // Desenha um rosto feliz, neutro ou triste dependendo do tipo
  // Rosto triste acabou não sendo utilizado
  display.drawCircle(FACE_X, FACE_Y, FACE_R, SSD1306_WHITE);
  display.fillCircle(FACE_X - EYE_OFFSET_X, FACE_Y - EYE_OFFSET_Y, EYE_R, SSD1306_WHITE);
  display.fillCircle(FACE_X + EYE_OFFSET_X, FACE_Y - EYE_OFFSET_Y, EYE_R, SSD1306_WHITE);

  if (type == 0) {
    display.drawLine(FACE_X - MOUTH_R, FACE_Y + 5, FACE_X + MOUTH_R, FACE_Y + 5, SSD1306_WHITE);
  } else if (type == -1) {
    drawArc(FACE_X, FACE_Y + 10, MOUTH_R, 180, 360);
  } else {
    drawArc(FACE_X, FACE_Y, MOUTH_R, 0, 180);
  }
}

void drawEnterScreen() {
  // Desenha tela de entrada de pessoa
  drawFace(1);
  display.setCursor(10, 5);
  display.print("Uma pessoa entrou!");
  drawInfoComponent();
  display.display();
}

void drawExitScreen() {
  // Desenha tela de saída de pessoa
  drawFace(0);
  display.setCursor(10, 5);
  display.print("Uma pessoa saiu!");
  drawInfoComponent();
  display.display();
}

void drawFooterComponent(int type) {
  // Desenha o rodapé com instruções dependendo do tipo
  if (type == 1) {
    display.setCursor(5, 55);
    display.setTextSize(1);
    display.setFont(&Picopixel);
    display.print("IO38: Menu");
    display.setFont();
  } else {
    display.setCursor(5, 55);
    display.setTextSize(1);
    display.setFont(&Picopixel);
    display.print("IO38: Selecionar  1: Navegar");
    display.setFont();
  }
}

void drawFilledRectangle(int x, int y, int w, int h, int color) {
  // Desenha um retângulo preenchido proporcional à quantidade de pessoas
  display.drawRect(x, y, w, h, color);
  if (people_count != 0) {
    int size = (people_count * 1.0 / people_max) * h;
    if (size > h) {
      size = h;
    }
    display.fillRect(x, y+(h-size), w, size, color);
  }
}

void drawInfoComponent() {
  // Desenha informações sobre a quantidade de pessoas atual, máxima e disponível
  display.setCursor(10, 20);
  display.print("Atual: ");
  display.print(String(people_count));
  display.setCursor(10, 30);
  display.print("Max.: ");
  display.print(String(people_max));
  display.setCursor(10, 40);
  display.print("Disp.: ");
  display.print(String(people_max - people_count));
}

void drawInfoScreen() {
  // Desenha a tela principal de informações
  drawFilledRectangle(90, 5, 20, 55, 1);
  drawInfoComponent();
  display.display();
}

Event checkEnteringOrLeaving() {
  // Verifica se uma pessoa entrou ou saiu com base nos sensores ultrassônicos
  bool s1 = (dist1 < min_distance);
  bool s2 = (dist2 < min_distance);

  switch (state) {
    case IDLE:
      // Nenhum sensor acionado - aguardando detecção inicial
      if (s1 && !s2) {
        // Sensor 1 detectou primeiro
        state = FIRST1;
      } else if (s2 && !s1) {
        // Sensor 2 detectou primeiro
        state = FIRST2;
      }
      break;

    case FIRST1:
      // Sensor 1 foi acionado primeiro
      if (s2) {
        // Normal: s1 depois s2 = ENTRAR, Invertido: s1 depois s2 = SAIR
        state = IDLE;
        return sensorsInverted ? LEAVE : ENTER;
      } else if (!s1) {
        // Pessoa voltou - reinicia para IDLE
        state = IDLE;
      }
      break;

    case FIRST2:
      // Sensor 2 foi acionado primeiro
      if (s1) {
        // Normal: s2 depois s1 = SAIR, Invertido: s2 depois s1 = ENTRAR
        state = IDLE;
        return sensorsInverted ? ENTER : LEAVE;
      } else if (!s2) {
        // Pessoa voltou - reinicia para IDLE
        state = IDLE;
      }
      break;
  }

  return NONE;
}

void drawMenuConfig() {
  // Desenha o menu de configuração
  display.clearDisplay();
  display.setCursor(3, 5);
  display.print("Menu de Configuracao");
  
  int yPos = 20;
  const char* options[] = {"Vagas", "Distancia", "Inverter", "Sair"};
  
  for (int i = 0; i < 4; i++) {
    if (i == menuSelection) {
      display.setCursor(5, yPos);
      display.print(">");
    }
    display.setCursor(15, yPos);
    display.print(options[i]);
    yPos += 10;
  }
  
  display.display();
}

void drawMenuVagas() {
  // Desenha o menu de ajuste de vagas
  display.clearDisplay();
  display.setCursor(5, 5);
  display.print("Ajuste de Vagas");
  
  display.setCursor(95, 20);
  display.print("Max:");
  display.setCursor(95, 30);
  display.print(people_max);
  
  int yPos = 25;
  const char* options[] = {"+1 Vaga", "-1 Vaga", "Sair"};
  
  for (int i = 0; i < 3; i++) { 
    if (i == menuSelection) {
      display.setCursor(5, yPos);
      display.print(">");
    }
    display.setCursor(15, yPos);
    display.print(options[i]);
    yPos += 10;
  }
  
  display.display();
}

void drawMenuDistancia() {
  // Desenha o menu de ajuste de distância
  display.clearDisplay();
  display.setCursor(10, 5);
  display.print("Ajuste de distancia");
  
  display.setCursor(95, 20);
  display.print("Dist:");
  display.setCursor(95, 30);
  display.print(min_distance);
  display.print("cm");
  
  int yPos = 25;
  const char* options[] = {"+10 cm", "-10 cm", "Sair"};
  
  for (int i = 0; i < 3; i++) {
    if (i == menuSelection) {
      display.setCursor(5, yPos);
      display.print(">");
    }
    display.setCursor(15, yPos);
    display.print(options[i]);
    yPos += 10;
  }
  
  display.display();
}

void drawMenuInverter() {
  // Desenha o menu de inverter sensores
  display.clearDisplay();
  display.setCursor(5, 5);
  display.print("Inverter Sensores");
  
  display.setCursor(95, 20);
  display.print("Est:");
  display.setCursor(95, 30);
  display.print(sensorsInverted ? "ON" : "OFF");
  
  int yPos = 25;
  const char* options[] = {"Alternar", "Sair"};
  
  for (int i = 0; i < 2; i++) {
    if (i == menuSelection) {
      display.setCursor(5, yPos);
      display.print(">");
    }
    display.setCursor(15, yPos);
    display.print(options[i]);
    yPos += 10;
  }
  
  display.display();
}

void handleMenuSelection() {
  // Trata a seleção do menu
  if (currentMenu == MENU_CONFIG) {
    if (menuSelection == 0) {
      // Seleciona o menu de ajuste de vagas
      currentMenu = MENU_VAGAS;
      menuSelection = 0;
      while (digitalRead(SELECT_BUTTON) == LOW) {
        delay(10);
      }
      lastSelectButtonState = HIGH;
    } else if (menuSelection == 1) {
      // Seleciona o menu de ajuste de distância
      currentMenu = MENU_DISTANCIA;
      menuSelection = 0;
      while (digitalRead(SELECT_BUTTON) == LOW) {
        delay(10);
      }
      lastSelectButtonState = HIGH;
    } else if (menuSelection == 2) {
      // Seleciona o menu de inverter sensores
      currentMenu = MENU_INVERTER;
      menuSelection = 0;
      while (digitalRead(SELECT_BUTTON) == LOW) {
        delay(10);
      }
      lastSelectButtonState = HIGH;
    } else if (menuSelection == 3) {
      // Sai do menu
      onMenu = false;
      onMainScreen = false;
      currentMenu = MENU_MAIN;
      menuSelection = 0;
    }
    
  } else if (currentMenu == MENU_VAGAS) {
    // Trata a seleção do menu de ajuste de vagas
    if (menuSelection == 0) {
      people_max += 1;
    } else if (menuSelection == 1) {
      if (people_max > 1) people_max -= 1;
    } else if (menuSelection == 2) {
      currentMenu = MENU_CONFIG;
      menuSelection = 0;
      while (digitalRead(SELECT_BUTTON) == LOW) {
        delay(10);
      }
      lastSelectButtonState = HIGH;
    }
    
  } else if (currentMenu == MENU_DISTANCIA) {
    // Trata a seleção do menu de ajuste de distância
    if (menuSelection == 0) {
      min_distance += 10;
    } else if (menuSelection == 1) {
      if (min_distance > 10) min_distance -= 10;
      else min_distance = 1;
    } else if (menuSelection == 2) {
      currentMenu = MENU_CONFIG;
      menuSelection = 0;
      while (digitalRead(SELECT_BUTTON) == LOW) {
        delay(10);
      }
      lastSelectButtonState = HIGH;
    }
  } else if (currentMenu == MENU_INVERTER) {
    // Trata a seleção do menu de inverter sensores
    if (menuSelection == 0) {
      sensorsInverted = !sensorsInverted;
    } else if (menuSelection == 1) {
      currentMenu = MENU_CONFIG;
      menuSelection = 0;
      while (digitalRead(SELECT_BUTTON) == LOW) {
        delay(10);
      }
      lastSelectButtonState = HIGH;
    }
  }
}

void updateMenu() {
  // Atualiza o menu com base nas entradas dos botões
  bool button1State = digitalRead(BUTTON_1);
  bool selectButtonState = digitalRead(SELECT_BUTTON);
  unsigned long currentTime = millis();
  
  if (button1State == LOW && lastButton1State == HIGH && (currentTime - lastButtonPress) > debounceDelay) {
    // Botão 1 foi pressionado 
    lastButtonPress = currentTime;
    
    if (currentMenu == MENU_CONFIG) {
      menuSelection = (menuSelection + 1) % 4;
    } else if (currentMenu == MENU_VAGAS || currentMenu == MENU_DISTANCIA) {
      menuSelection = (menuSelection + 1) % 3;
    } else if (currentMenu == MENU_INVERTER) {
      menuSelection = (menuSelection + 1) % 2;
    }
  }
  
  if (selectButtonState == LOW && lastSelectButtonState == HIGH && (currentTime - lastButtonPress) > debounceDelay) {
    lastButtonPress = currentTime;
    handleMenuSelection();
  }
  
  lastButton1State = button1State;
  lastSelectButtonState = selectButtonState;
  
  if (currentMenu == MENU_CONFIG) {
    drawMenuConfig();
  } else if (currentMenu == MENU_VAGAS) {
    drawMenuVagas();
  } else if (currentMenu == MENU_DISTANCIA) {
    drawMenuDistancia();
  } else if (currentMenu == MENU_INVERTER) {
    drawMenuInverter();
  }

  drawFooterComponent(2);
}



void loop() {
  if (onMenu) {
    // Se estiver no menu, não roda qualquer outra lógica, apenas do menu
    updateMenu();
    delay(100);
    return;
  }
  
  // Lê o estado do botão SELECT
  bool selectButtonState = digitalRead(SELECT_BUTTON);
  unsigned long currentTime = millis();
  
  if (selectButtonState == LOW && lastSelectButtonState == HIGH && 
      (currentTime - lastButtonPress) > debounceDelay) {
    // Botão SELECT foi pressionado
    lastButtonPress = currentTime;
    onMenu = true;
    currentMenu = MENU_CONFIG;
    menuSelection = 0;
    
    while (digitalRead(SELECT_BUTTON) == LOW) {
      delay(10);
    }
    lastSelectButtonState = HIGH;
    return;
  }
  lastSelectButtonState = selectButtonState;
  
  // Atualiza as distâncias dos sensores
  getDistance();

  // Verifica se alguém entrou ou saiu
  Event ev = checkEnteringOrLeaving();

  if (people_count >= people_max) {
    // Acende o LED vermelho se o número máximo de pessoas for atingido
    digitalWrite(LED_RED, HIGH);
  } else {
    // Apaga o LED vermelho se o número máximo de pessoas não for atingido
    digitalWrite(LED_RED, LOW);
  }

  if (ev == ENTER) {
    // Se alguém entrou, liga o LED verde e atualiza a tela
    people_count++;
    digitalWrite(LED_GREEN, HIGH);

    onMainScreen = false;
    
    display.clearDisplay();
    drawEnterScreen();
    delay(2000);
    digitalWrite(LED_GREEN, LOW);
  } else if (ev == LEAVE) {
    // Se alguém saiu, liga o LED azul e atualiza a tela
    if (people_count > 0) {
      people_count--;
    }
    digitalWrite(LED_BLUE, HIGH);

    onMainScreen = false;
    
    display.clearDisplay();
    drawExitScreen();
    delay(2000);
    digitalWrite(LED_BLUE, LOW);
  } else if (!onMainScreen) {
    // Se o estado não se encontra em nenhum outro e não na tela principal, 
    // desenha a tela principal de informações
    display.clearDisplay();
    drawFooterComponent(1);
    onMainScreen = true;
    drawInfoScreen();
    delay(100);
  }

  delay(100);
}
