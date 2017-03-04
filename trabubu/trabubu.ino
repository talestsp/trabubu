// Programa : Temp. Hum. Control
// Author : Tales Pimentel

///////////////////////
// LIBS
#include "Wire.h"
#define DS1307_ADDRESS 0x68
#include <LiquidCrystal_I2C.h>
byte zero = 0x00; 
#include <SD.h>
#include <DHT.h>

//////////////////////
// PINS
const short SD_CS_PIN = 10;
const short DHT_PIN = 7;
const short FAN_PIN = 8;
const short MODE_PIN = 0;
const short LCD_LIGHT_PIN = 1;
const short LCD_SWITCH_PIN = 2;


//////////////////////
// LCD
LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7,3, POSITIVE);

//////////////////////
// SD
const String DATA_FILEPATH = "Data.csv";
const char HEADER[58] = "TS,MODE,TEMP,UMID,EXAUST,FAN_ON,FAN_SLP,UMID_T,FAN_SLP_T";
File data_file;

//////////////////////
// DHT
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);


//////////////////////
// GLOBAL VARIABLES
float const DELAY_N = 50;
bool show_th = true;
short UMID_THRESH = 85;
short FAN_SLEEP_THRESH = 18000;
long fan_sleep_counter = FAN_SLEEP_THRESH - 10;
long fan_active = 0;
String timestamp;
String last_timestamp;

void setup()
{
  Serial.begin(9600);
  
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);
  
  pinMode(MODE_PIN, INPUT);
  
  pinMode(LCD_LIGHT_PIN, INPUT);
  digitalWrite(LCD_LIGHT_PIN, HIGH);
  
  pinMode(LCD_SWITCH_PIN, INPUT);
  
  setup_rtc();
  Serial.println(timeRTC());
  setup_lcd();
  setup_sd();
  setup_datafile();
  
  //set_rtc_time();
  setup_dht();
  setup_lcd_t_u();

}

void loop(){
  timestamp = timeRTC();
  
  while(timestamp == last_timestamp){
    timestamp = timeRTC();
    if (digitalRead(LCD_SWITCH_PIN)){
      lcd_show_time(timestamp);
      delay(500);
    }
    delay(DELAY_N);
  }
  
  last_timestamp = timestamp;
  float humid = dht.readHumidity();
  float temp = dht.readTemperature();

  if (show_th == false){
    setup_lcd_t_u();
    show_th = true;
  }

  show_data_temp_humid(temp, humid);
  
  if (digitalRead(MODE_PIN)){
    control_fan(humid);
    lcd.setCursor(15,1);
    lcd.print("C");
  } else{
    lcd.setCursor(15,1);
    lcd.print("M");
  }

  write_data(temp, humid, timestamp);

}


//********************************
//********************************
// FUNCTIONS

void setup_lcd(){
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Iniciando...");
  delay(1500);
  
}

void setup_rtc(){
    Wire.begin();
}

void setup_sd(){
  SD.begin(SD_CS_PIN);
}

void setup_datafile(){
  if(! SD.exists(DATA_FILEPATH)){
    data_file = SD.open(DATA_FILEPATH, FILE_WRITE);
    if (data_file){
      Serial.println("Arq criado!");
    } else{
      Serial.println("Arq ERRO!");
    }
    data_file.println(HEADER);
    data_file.close();
  } else{
    Serial.println("Arq existe!");
  }
}

void setup_dht(){
  lcd.setCursor(0,1);
  dht.begin();
}

void control_fan(float humid){  
  if (humid >= UMID_THRESH && fan_sleep_counter > FAN_SLEEP_THRESH){
    digitalWrite(FAN_PIN, LOW);//liga o relé
      
  }else if (humid < UMID_THRESH){
    digitalWrite(FAN_PIN, HIGH);//desliga o relé
    
  }

  //dealing with overflow
  if (fan_sleep_counter < 0){
    fan_sleep_counter = FAN_SLEEP_THRESH;
  }

  if (digitalRead(FAN_PIN) == LOW){
    fan_active++;
    fan_sleep_counter = 0;
  } else {
    fan_active = 0;
    fan_sleep_counter++;
  }
}

void show_data_temp_humid(float t, float h){
    if (isnan(h) || isnan(t))
  {
    Serial.println("Falha DHT!");
  }
  
  if (isnan(t)){
    lcd.setCursor(6,0);
    lcd.print("fail");
  }else{
    lcd.setCursor(6,0);
    lcd.print(t);
    lcd.setCursor(10,0);
    lcd.print(" ");
  }
  
  if (isnan(h)){
    lcd.setCursor(6,1);
    lcd.print("fail");
  }else{
    lcd.setCursor(6,1);
    lcd.print(h);
    lcd.setCursor(10,1);
    lcd.print(" ");
  }

  if (digitalRead(LCD_LIGHT_PIN)){
    lcd.setBacklight(HIGH);
  } else {
    lcd.setBacklight(LOW);
  }

  if (timestamp.endsWith("0") || timestamp.endsWith("2") || timestamp.endsWith("4") || timestamp.endsWith("6") || timestamp.endsWith("8")) {
    lcd.setCursor(4,0);
    lcd.print(":");
    lcd.setCursor(4,1);
    lcd.print(":");
  } else {
    lcd.setCursor(4,0);
    lcd.print(" ");
    lcd.setCursor(4,1);
    lcd.print(" ");
  }

}


void setup_lcd_t_u(){
  if (show_th == false) {
    lcd.clear();  
  }
  
  byte grau[8] ={ B00001100, 
                B00010010, 
                B00010010, 
                B00001100, 
                B00000000, 
                B00000000, 
                B00000000, 
                B00000000,};
                
  // Cria o caracter customizado com o simbolo do grau
  lcd.createChar(0, grau); 
  // Informacoes iniciais no display
  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.setCursor(11,0);
  // Mostra o simbolo do grau
  lcd.write(byte(0));
  lcd.print("C");
  lcd.setCursor(0,1);
  lcd.print("Umid: ");
  lcd.setCursor(11,1);
  lcd.print("%");
}

void write_data(float temp, float humid, String timestamp){
  data_file = SD.open(DATA_FILEPATH, FILE_WRITE);
  //TS,MODE,TEMP,UMID,EXAUST,FAN_ON,FAN_SLP,UMID_T,FAN_SLP_T
  if (data_file){
      data_file.print(timestamp);
      data_file.print(",");
      
      if (digitalRead(MODE_PIN)){
        data_file.print("Control");  
      } else{
        data_file.print("Monitor");
      }
      
      data_file.print(",");
      data_file.print(temp);
      data_file.print(",");
      data_file.print(humid);
      data_file.print(",");
      
      if (digitalRead(FAN_PIN) == LOW){
        data_file.print("ON");
      } else{
        data_file.print("OFF");
      }
      
      data_file.print(",");
      data_file.print(fan_active);
      data_file.print(",");
      data_file.print(fan_sleep_counter);
      data_file.print(",");
      data_file.print(UMID_THRESH);
      data_file.print(",");
      data_file.print(FAN_SLEEP_THRESH);
      
      data_file.println("");
      
      lcd.setCursor(14,0);
      lcd.print("sd");
  } else{
      lcd.setCursor(14,0);
      lcd.print("ff");
  }
  data_file.close();
}

String timeRTC(){
  
  // Le os valores (data e hora) do modulo DS1307
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS, 7);
  int segundos = to_decimal(Wire.read());
  int minutos = to_decimal(Wire.read());
  int horas = to_decimal(Wire.read() & 0b111111);
  int diadasemana = to_decimal(Wire.read()); 
  int diadomes = to_decimal(Wire.read());
  int mes = to_decimal(Wire.read());
  int ano = to_decimal(Wire.read());

  String timestamp = "";
  
  timestamp.concat("20");
  timestamp.concat(ano);
  timestamp.concat("/");
  
  if (mes < 10) {
    timestamp.concat("0");
  }
  timestamp.concat(mes);
  timestamp.concat("/");

  if (diadomes < 10) {
    timestamp.concat("0");
  }
  timestamp.concat(diadomes);
  timestamp.concat(" ");

  if (horas < 10) {
    timestamp.concat("0");
  }
  timestamp.concat(horas);
  timestamp.concat(":");

  if (minutos < 10){
    timestamp.concat("0");  
  }
  timestamp.concat(minutos);
  timestamp.concat(":");

  if (segundos < 10){
    timestamp.concat("0");
  }
  timestamp.concat(segundos);

  return timestamp;

}

void lcd_show_time(String timestamp){
  show_th = false;
  lcd.clear();
  
  String yyyy = timestamp.substring(0,4);
  String mm = timestamp.substring(5,7);
  String dd = timestamp.substring(8,10);
  lcd.setCursor(0,0);
  lcd.print(" " + dd + " / " + mm + " / " + yyyy);

  String hours = timestamp.substring(11,13);
  String minutes = timestamp.substring(14,16);
  String seconds = timestamp.substring(17,19);
  lcd.setCursor(0,1);
  lcd.print("    " + hours + ":" + minutes + ":" + seconds + "    ");
}

void set_rtc_time()   //Seta a data e a hora do DS1307
{
  byte segundos = 30; //Valores de 0 a 59
  byte minutos = 37; //Valores de 0 a 59
  byte horas = 15; //Valores de 0 a 23
  byte diadasemana = 6; //Valores de 0 a 6 - 0=Domingo, 1 = Segunda, etc.
  byte diadomes = 25; //Valores de 1 a 31
  byte mes = 2; //Valores de 1 a 12
  byte ano = 17; //Valores de 0 a 99
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); //Stop no CI para que o mesmo possa receber os dados

  //As linhas abaixo escrevem no CI os valores de 
  //data e hora que foram colocados nas variaveis acima
  Wire.write(to_bcd(segundos));
  Wire.write(to_bcd(minutos));
  Wire.write(to_bcd(horas));
  Wire.write(to_bcd(diadasemana));
  Wire.write(to_bcd(diadomes));
  Wire.write(to_bcd(mes));
  Wire.write(to_bcd(ano));
  Wire.write(zero);
  Wire.endTransmission(); 
}

byte to_bcd(byte val)
{ 
  //Converte o número de decimal para BCD
  return ( (val/10*16) + (val%10) );
}

byte to_decimal(byte val)  
{ 
  //Converte de BCD para decimal
  return ( (val/16*10) + (val%16) );
}
