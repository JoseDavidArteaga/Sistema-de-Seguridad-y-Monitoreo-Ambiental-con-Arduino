/*
PINES
>> LCD 
D7(2) - D6(3) - D5(4) - D4(5) - E(11) - RS(12)
>> LED 
R(A15)-G(A14)-B(A13)
>> DHT11 - A12
>> PHOTOCELL - A11
>> HALL - A10
>> BUZZER - A9
>> BUTTON - A8

*/

// MAQUINA DE ESTADOS
#include "StateMachineLib.h"
#include <LiquidCrystal.h>
#include <Keypad.h>

// Definición de estados
enum State
{
   Inicio = 0,
   Bloqueo = 1,
   Config = 2,
   MonitoreoAmbiental = 3,
   MonitorEventos = 4,
   Alarma = 5
};

// Definición de entradas
enum Input
{
   Reset = 0,
   CorrectPassword = 1,
   IncorrectKey = 2,
   PressButton = 3,
   Unknown = 4,
};

StateMachine stateMachine(6, 12); // 6 estados, 12 transiciones
Input input = Unknown;
unsigned long bloqueoStartTime; // Para el timeout del estado de bloqueo
const unsigned long tiempoBloqueo = 10000; // 10 segundos de bloqueo
unsigned long monitoreoAmbientalStartTime; // Para el timeout del estado MonitoreoAmbiental
unsigned long monitorEventosStartTime; // Para el timeout del estado MonitorEventos
unsigned long alarmaStartTime; // Para el timeout del estado alarma

// Configuración LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Configuración Keypad
const byte ROWS = 4; // Cuatro filas
const byte COLS = 4; // Cuatro columnas
char keys[ROWS][COLS] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};
byte rowPins[ROWS] = {22, 24, 26, 28}; // Conectar a las salidas de fila del keypad
byte colPins[COLS] = {30, 32, 34, 36}; // Conectar a las salidas de columna del keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//Creo un simbolo personalizado de una flecha
const byte flecha[] = {
  B00000,
  B00100,
  B01100,
  B11111,
  B11111,
  B01100,
  B00100,
  B00000
};

// Configuración LED RGB
const int LED_RED = A15, LED_GREEN = A14, LED_BLUE = A13; // Pines del LED RGB

void ALL_LEDS(bool state){
  RED_LED_STATE(state);
  BLUE_LED_STATE(state);
  GREEN_LED_STATE(state);
}
void RED_LED_STATE(bool state){
  digitalWrite(LED_RED, state);
}
void BLUE_LED_STATE(bool state){
  digitalWrite(LED_BLUE, state);
}
void GREEN_LED_STATE(bool state){
  digitalWrite(LED_GREEN, state);
}


//CONFIG BUTTON 
#define PULSADOR A8      // pulsador en pin A8

// Configuración Buzzer
#define BUZZER_PASIVO A9 // buzzer pasivo en pin 38
#define beat 500        // = 60 s / 120 bpm * 1000 ms
#define NOTE_D4  294
#define NOTE_D5  587
#define NOTE_A4  440
#define NOTE_GS4 415
#define NOTE_G4  392
#define NOTE_F4  349
#define NOTE_B3  247
#define NOTE_D6  1175
#define NOTE_A5  880
#define NOTE_GS5 831
#define NOTE_G5  784
#define NOTE_F5  698
#define NOTE_E5  659
#define NOTE_D7  2349
#define NOTE_C6  1047
#define NOTE_A6  1760
#define NOTE_G6  1568
#define NOTE_D8  4699
#define NOTE_C7  2093

int melodiaBloqueo[] = {
  NOTE_D4, NOTE_D4, NOTE_D5, 0, NOTE_A4, 0, NOTE_GS4, 0, NOTE_G4, 0, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
  NOTE_B3, NOTE_B3, NOTE_D5, 0, NOTE_A4, 0, NOTE_GS4, 0, NOTE_G4, 0, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
  NOTE_D5, NOTE_D5, NOTE_D6, 0, NOTE_A5, 0, NOTE_GS5, 0, NOTE_G5, 0, NOTE_F5, NOTE_D5, NOTE_F5, NOTE_G5,
  NOTE_F5, NOTE_F5, NOTE_F5, 0, NOTE_F5, 0, NOTE_E5, NOTE_F5, NOTE_D5, NOTE_D5, NOTE_D5
};

double duracionesBloqueo[] = {
  0.25, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25,
  0.25, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25,
  0.25, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25,
  0.5,  0.25, 0.25, 0.25, 0.15, 0.10, 0.5,  0.5, 1.25, 0.25, 0.25
};

int melodiaCorrectKey[] = {
  NOTE_F5, NOTE_F5, NOTE_F5, 0, NOTE_G5, 0, NOTE_GS5, NOTE_G5, NOTE_F5, NOTE_D5, NOTE_F5, NOTE_G5, 0,
  NOTE_F5, NOTE_F5, NOTE_F5, 0, NOTE_G5, 0, NOTE_GS5, 0, NOTE_A5, 0, NOTE_C6, 0, NOTE_A5,
  NOTE_D6, 0, NOTE_D6, 0, NOTE_D6, NOTE_A5, NOTE_D6, NOTE_C6, NOTE_G6
};

double duracionesCorrectKey[] = {
  0.5,  0.25, 0.25, 0.25, 0.25, 0.5,0.083,0.083,0.084, 0.25,0.25, 0.25, 0.5,
  0.5, 0.25, 0.25, 0.25, 0.25, 0.25, 0.5, 0.75, 0.25, 0.25, 0.25, 0.25, 0.25,
  0.25, 0.25, 0.25, 0.25, 1, 0.5, 0.25, 0.25, 0.75
};

void play(int note, double note_val) {
  tone(BUZZER_PASIVO, note);
  delay((beat * note_val) - 20);
  noTone(BUZZER_PASIVO);
  delay(20);
}

void rest(double note_val) {
  delay(beat * note_val);
}


//Configuracion de sensores 
#include "DHT.h"
const int  DHTPIN=  A12;
#define DHTTYPE DHT11   // DHT 11 
DHT dht(DHTPIN, DHTTYPE);
//Sensor de luz
const int photocellPin = A11;
//Sensor de efectoHall
const int efectoHallPin = A10;

//>>>> valores reset (por defecto)
const float RESETLIGHT_HIGH = 3;
const float RESETLIGHT_LOW = 4;
const float RESETHALL = 5; 
const float RESETTEMP_HIGH = 1;
const float RESETTEMP_LOW = 2;

//variables para almacenar las lecturas actuales de los sensores
float valuehall_high = RESETHALL;
float valuelight_high= RESETLIGHT_HIGH;
float valuetemp_high =RESETTEMP_HIGH;
float valuetemp_low= RESETTEMP_LOW; 
float valuelight_low= RESETLIGHT_LOW;//limites que se pueden cambiar en el modo de edicion

float valuelight, valuehall, valuetemp, valuehum;//estos son los valores que cambian, donde se guardan lo que se lee de los pines
String valornuevo= "";

//la funcion de alertar deberia ser algo como 
// if(valuelight > valuelight_high){ alertar} 

void readLuz(void){
  long prevtime=micros();
  valuelight = analogRead(photocellPin);
  Serial.print("Luz: ");
  Serial.println(valuelight);
  long currenttime=micros();
  long diftime = currenttime;
}

void readHall(void){
  long prevtime=micros();
  valuehall = analogRead(efectoHallPin);
  Serial.print("Mag: ");
  Serial.println(valuehall);
  long currenttime=micros();
  long diftime = currenttime;
}

void readHumedad(void){
  long prevtime=micros();
  valuehum = dht.readHumidity();
  Serial.print("Humedad: ");
  Serial.println(valuehum);
  long currenttime=micros();
  long diftime = currenttime;
}

void readTemperatura(void){
  long prevtime=micros();
  valuetemp = dht.readTemperature();
  Serial.print("Temperatura: ");
  Serial.println(valuetemp);
  long currenttime=micros();
  long diftime = currenttime;
}

//confing tasks
#include "AsyncTaskLib.h"
AsyncTask TaskLuz(1000, true, readLuz);
AsyncTask TaskHall(1500, true, readHall);
AsyncTask TaskHumedad(2200, true, readHumedad);
AsyncTask TaskTemperatura(2500, true, readTemperatura);

/*
 * CONFING LIQUID MENU
 */
#include <LiquidMenu.h>


LiquidLine tempHigh_line00(0,0,"TH-Temp-High"); 
LiquidLine tempHigh_line01(0,1,"TH-Temp-High");

LiquidLine tempLow_line00(0,0,"TH-Temp-Low");
LiquidLine tempLow_line01(0,1,"TH-Temp-Low");

LiquidLine LuzHigh_line00(0,0,"TH-Luz-High");
LiquidLine LuzHigh_line01(0,1,"TH-Luz-High");

LiquidLine LuzLow_line00(0,0,"TH-Luz-Low");
LiquidLine LuzLow_line01(0,1,"TH-Luz-Low");

LiquidLine Hall00(0,0,"TH-Hall");
LiquidLine Hall01(0,1,"TH-Hall");

LiquidLine Reset00(0,0,"RESET");
LiquidLine Reset01(0,1,"RESET");

LiquidScreen screenTempHL(tempHigh_line00, tempLow_line01);
LiquidScreen screenTempL_LuzH(tempLow_line00, LuzHigh_line01);
LiquidScreen screenLuzHL(LuzHigh_line00,LuzLow_line01);
LiquidScreen screenLuzL_Hall(LuzLow_line00, Hall01);
LiquidScreen screenHall_RST(Hall00,Reset01);
LiquidScreen screenRST_TempH (Reset00, tempHigh_line01);

LiquidMenu menu(lcd);//SE CREA EL MENU

LiquidLine ShowValue00 (0,0,"Show Value");
LiquidLine ShowValue01 (0,1,"Show Value");

LiquidLine EditValue00 (0,0,"Edit Value");
LiquidLine EditValue01 (0,1,"Edit Value");

LiquidLine return00 (0,0,"return");
LiquidLine return01 (0,1,"return");

LiquidScreen screenShow_Edit(ShowValue00,EditValue01);
LiquidScreen screenEdit_return(EditValue00, return01);
LiquidScreen screenReturn_Show(return00, ShowValue01);

LiquidMenu menuEdit(lcd);

/*
 * FUNCIONES PARA EL MOVIMIENTO DEL MENU
 */

bool editing = false;
bool editMode = true;
String typeValue = "";
void moveMenu(){
  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
    // Check all the buttons
    if (key == 'A') {
      menu.previous_screen();
      menu.set_focusedLine(0);
      menu.update();
    }
    if (key == 'B') {
      menu.next_screen();
      menu.set_focusedLine(0);
      menu.update();
    }
    if (key == 'C') {
      editing = true;
      menu.call_function(1);
      lcd.clear();
    }
  }
}
void moveMenuEdit(){
  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
    // Check all the buttons
    if (key == 'A') {
      menuEdit.previous_screen();
      menuEdit.set_focusedLine(0);
      menuEdit.update();
    }
    if (key == 'B') {
      menuEdit.next_screen();
      menuEdit.set_focusedLine(0);
      menuEdit.update();
    }
    if (key == 'C') {
      Serial.println(F("Funcion llamada"));
      menuEdit.call_function(2);
      lcd.clear();
    }

  }
}
void menuLoop() {
  if(!editing){
    menu.softUpdate();
    moveMenu();
  }else{
    menuEdit.softUpdate();
    moveMenuEdit();
  }
}
//MONITOREO AMBIENTAL 
void mostrarValores(){
    TaskLuz.Update();
    TaskHall.Update();
    TaskHumedad.Update();
    TaskTemperatura.Update();
    lcd.setCursor(0, 0);
    lcd.print("TEM:");
    lcd.print(valuetemp);
    lcd.setCursor(7,0);
    lcd.print(" HUM:");
    lcd.print(valuehum);
    lcd.setCursor(0, 1);
    lcd.print("LGT:");
    lcd.print(valuelight);
    lcd.setCursor(7,1);
    lcd.print(" MAG:");
    lcd.print(valuehall);
}

//MENU CONFIGURACION
int leerNumero() {
  valornuevo = ""; // Reset the input string at the start
  while (true) {
    char key = keypad.getKey();
    if (key) {
      Serial.println(key);
      if (key == 'C') {
        float result = valornuevo.toFloat();
        valornuevo = ""; // Reset the input string after reading
        return result;
      } else {
        valornuevo += key;
        lcd.print(key);
      }
    }
    delay(50); // Small delay to debounce the key press
  }
}
void editScreen(){
  lcd.setCursor(0, 0);
  lcd.print("Insert value:");
  lcd.setCursor(10, 0);
  lcd.setCursor(0, 1);
  lcd.print(">>");
  lcd.setCursor(3, 1);
}


// Configuración inicio sesión
char CLAVE[7]; // Buffer para la clave ingresada, más el terminador nulo
char CLAVE_MAESTRA[7] = "123456";
byte INDICE = 0;
int NUMERO_INTENTOS = 0;
bool isAuthenticated = false;

Input login(){
  ALL_LEDS(false);
  char key = keypad.getKey();
  if (NUMERO_INTENTOS >= 3) {
    lcd.clear();
    NUMERO_INTENTOS = 0;
    return IncorrectKey;
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Ingrese clave:");
    // Lleno la clave
    if (key) {
      if (INDICE < 6) {
        CLAVE[INDICE] = key;
        INDICE++;
        lcd.setCursor(INDICE, 1);
        lcd.print('*'); 
      }
      // Verifico si las claves son iguales
      if (INDICE == 6) {
        CLAVE[INDICE] = '\0'; // Añadir terminador nulo
        lcd.setCursor(0, 1);
        if (!strcmp(CLAVE, CLAVE_MAESTRA)) {
          lcd.print("Clave correcta");
          GREEN_LED_STATE(true);
          isAuthenticated = true; // Set authenticated to true
          delay(2000); // Espera 2 segundos
          lcd.clear();
          return CorrectPassword;
        } else {
          lcd.print("Clave incorrecta");
          BLUE_LED_STATE(true);
          NUMERO_INTENTOS++;
          delay(2000); // Espera 2 segundos
          lcd.clear();
          lcd.setCursor(0, 0);
          INDICE = 0; // Reiniciar el índice para la siguiente clave
          return IncorrectKey;
        }
      }
    }
  }
  return Unknown;
}

void setupStateMachine()
{
   // Transiciones entre estados
   stateMachine.AddTransition(Inicio, Bloqueo, []() { return input == IncorrectKey && NUMERO_INTENTOS >= 3; });
   stateMachine.AddTransition(Inicio, Config, []() { return input == CorrectPassword; });

   stateMachine.AddTransition(Bloqueo, Inicio, []() { return (millis() - bloqueoStartTime) >= 10000; });

   stateMachine.AddTransition(Config, MonitoreoAmbiental, []() { return input == PressButton; });

   stateMachine.AddTransition(MonitoreoAmbiental, Config, []() { return input == PressButton; });
   stateMachine.AddTransition(MonitoreoAmbiental, MonitorEventos, []() { return (millis() - monitoreoAmbientalStartTime) >= 7000; });
   stateMachine.AddTransition(MonitoreoAmbiental, Alarma, []() { return input == PressButton; });

   stateMachine.AddTransition(MonitorEventos, Config, []() { return input == PressButton; });
   stateMachine.AddTransition(MonitorEventos, MonitoreoAmbiental, []() { return (millis() - monitorEventosStartTime) >= 3000; });
   stateMachine.AddTransition(MonitorEventos, Alarma, []() { return input == PressButton; });

   stateMachine.AddTransition(Alarma, Inicio, []() { return input == PressButton; });
   stateMachine.AddTransition(Alarma, MonitoreoAmbiental, []() { return (millis() - alarmaStartTime) >= 4000; });

   // Acciones al entrar en cada estado
   stateMachine.SetOnEntering(Inicio, outputInicio);
   stateMachine.SetOnEntering(Bloqueo, outputBloqueo);
   stateMachine.SetOnEntering(Config, outputConfig);
   stateMachine.SetOnEntering(MonitoreoAmbiental, outputMonitoreoAmbiental);
   stateMachine.SetOnEntering(MonitorEventos, outputMonitorEventos);
   stateMachine.SetOnEntering(Alarma, outputAlarma);

   // Acciones al salir de cada estado
   stateMachine.SetOnLeaving(Inicio, []() { ALL_LEDS(false); });
   stateMachine.SetOnLeaving(Bloqueo, []() { 
    ALL_LEDS(false); });
}

void setup() 
{
  ALL_LEDS(false);
	// set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  Serial.begin(9600);
  lcd.createChar(3, flecha);

   //buzzer configuration 
  pinMode(BUZZER_PASIVO, OUTPUT);
  //LED CONFIGURACION 
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  //DHT BEGIN
  dht.begin();

  //task.Start()

  TaskLuz.Start();
  TaskHall.Start();
  TaskHumedad.Start();
  TaskTemperatura.Start();

	//MENU CONFIGURACION
  menu.init();
  tempHigh_line00.set_focusPosition(Position::RIGHT); 
  tempLow_line00.set_focusPosition(Position::RIGHT);
  LuzHigh_line00.set_focusPosition(Position::RIGHT);
  LuzLow_line00.set_focusPosition(Position::RIGHT);
  Hall00.set_focusPosition(Position::RIGHT);
  Reset00.set_focusPosition(Position::RIGHT);
/*
  tempHigh_line00.attach_function(1, fn_temphigh);
  tempLow_line00.attach_function(1, fn_templow);
  LuzHigh_line00.attach_function(1, fn_luzhigh);
  LuzLow_line00.attach_function(1, fn_luzlow);
  Hall00.attach_function(1, fn_hall);
  Reset00.attach_function(1, fn_reset);
*/
	menu.add_screen(screenTempHL);
	menu.add_screen(screenTempL_LuzH);
	menu.add_screen(screenLuzHL);
  menu.add_screen(screenLuzL_Hall);
  menu.add_screen(screenHall_RST);
	menu.add_screen(screenRST_TempH );
  menu.set_focusedLine(0);

  menuEdit.init();

  ShowValue00.set_focusPosition(Position::RIGHT);
  EditValue00.set_focusPosition(Position::RIGHT);
  return00.set_focusPosition(Position::RIGHT);
/*
  ShowValue00.attach_function(2, fn_showValue);
  EditValue00.attach_function(2, fn_editValue);
  return00.attach_function(2, fn_return);
*/
  menuEdit.add_screen(screenShow_Edit);
  menuEdit.add_screen(screenEdit_return);
  menuEdit.add_screen(screenReturn_Show);

  lcd.clear();//ojito

  Serial.println("Starting State Machine...");
  setupStateMachine();   
  Serial.println("State Machine Started");

  stateMachine.SetState(Inicio, false, true);
}

void loop() 
{
   if (static_cast<State>(stateMachine.GetState()) == Inicio) {
       input = login();
   } else {
       input = static_cast<Input>(readInput());
   }

   stateMachine.Update();
}

int readInput()
{
   Input currentInput = Unknown;
   if (Serial.available())
   {
      char incomingChar = Serial.read();

      switch (incomingChar)
      {
         case 'R': currentInput = Reset;           break;
         case 'C': currentInput = CorrectPassword; break; // Simula una contraseña correcta
         case 'K': currentInput = IncorrectKey;    break; // Simula una clave incorrecta
         case 'B': currentInput = PressButton;     break; // Simula un botón presionado
         default: break;
      }
   }

   return currentInput;
}

void outputInicio()
{
  Serial.println("Estado: Inicio");
  Serial.println();
  // Aquí no llamamos a login directamente, se maneja en loop()
}

void outputBloqueo()
{
   Serial.println("Estado: Bloqueo");
   RED_LED_STATE(true); // Enciende el LED rojo
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("SISTEMA");
  lcd.setCursor(0, 1);
   lcd.print("BLOQUEADO");
   bloqueoStartTime = millis(); // Inicia el tiempo de bloqueo

  // Reproducir la melodía de bloqueo durante 5 segundos
  unsigned long currentTime = millis();
  while (millis() - currentTime < 5000) {
    for (int i = 0; i < sizeof(melodiaBloqueo) / sizeof(int); i++) {
      if (melodiaBloqueo[i] == 0) {
        rest(duracionesBloqueo[i]);
      } else {
        play(melodiaBloqueo[i], duracionesBloqueo[i]);
      }
    }
  }
}

void outputConfig()
{
   Serial.println("Estado: Configuración");
   RED_LED_STATE(true); // Enciende el LED rojo
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("BIENVENIDO");
   lcd.setCursor(0, 1);
   lcd.print(":D");
   bloqueoStartTime = millis(); // Inicia el tiempo de bloqueo

  // Reproducir la melodía de contraseña correcta durante 3 segundos
  unsigned long currentTime = millis();
  while (millis() - currentTime < 3000) {
    for (int i = 0; i < sizeof(melodiaCorrectKey) / sizeof(int); i++) {
      if (melodiaCorrectKey[i] == 0) {
        rest(duracionesCorrectKey[i]);
      } else {
        play(melodiaCorrectKey[i], duracionesCorrectKey[i]);
      }
    }
  }

  // Inicia el menú de configuración
  lcd.clear();
  menu.init(); // Inicializa el menú principal
  editing = false; // Empieza en el menú principal, no en modo de edición
  menu.change_screen(1); // Cambia a la primera pantalla del menú
  menu.set_focusedLine(0); // Enfoca la primera línea
}

void outputMonitoreoAmbiental()
{
   Serial.println("Estado: Monitoreo Ambiental");
   monitoreoAmbientalStartTime = millis(); // Inicia el tiempo de monitoreo ambiental
   Serial.println();
}

void outputMonitorEventos()
{
   Serial.println("Estado: Monitor eventos");
   monitorEventosStartTime = millis(); // Inicia el tiempo de monitor de eventos
   Serial.println();
}

void outputAlarma()
{
   Serial.println("Estado: Alarma");
   alarmaStartTime = millis(); // Inicia el tiempo de alarma
   Serial.println();
}
