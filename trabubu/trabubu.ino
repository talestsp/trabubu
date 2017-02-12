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
const int SD_CS_PIN = 10;
const int DHT_PIN = 9;
const int PIN_FAN = 8;

//////////////////////
// LCD
LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7,3, POSITIVE);

//////////////////////
// SD
const String DATA_FILEPATH = "Datalog.csv";
const char HEADER[50] = "TIMESTAMP,TEMPERATURA,UMIDADE";
File data_file;

//////////////////////
// DHT
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

//////////////////////
// GLOBAL VARIABLES
int const DELAY_N = 1000;
int const UMID_THRESH = 60;
int const FAN_SLEEP_THRESH = 45;
int fan_sleep_counter = 0;


void setup()
{
  Serial.begin(9600);
  pinMode(PIN_FAN, OUTPUT);
  
  setup_rtc();
  setup_lcd();
  setup_sd();
  setup_datafile();
  //SelecionaDataeHora();
  setup_dht();
  setup_lcd_t_u();

}

void loop(){
  delay(DELAY_N);
  
  Serial.println("\n");
  Serial.println("*******************");

  Serial.println(timeRTC());
  
  fan_sleep_counter = fan_sleep_counter + (DELAY_N / 1000);

  float humid = dht.readHumidity();
  float temp = dht.readTemperature();
  String timestamp = timeRTC();
  show_data_temp_humid(temp, humid);
  write_data(temp, humid, timestamp);
  control_fan(humid);
  
  
}


//********************************
//********************************
// FUNCTIONS

void setup_lcd(){
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  
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
      Serial.println("Arquivo de dados criado!");
    } else{
      Serial.println("Arquivod e dados ERRO!");
    }
    data_file.println(HEADER);
    data_file.close();
  } else{
    Serial.println("Arquivo de dados jah existe!");
  }
}

void setup_dht(){
  lcd.setCursor(0,1);
  dht.begin();
}

void control_fan(float humid){
  Serial.print("fan_sleep_counter: ");
  Serial.println(fan_sleep_counter);
  if (humid >= UMID_THRESH && fan_sleep_counter > FAN_SLEEP_THRESH){
    digitalWrite(PIN_FAN, HIGH);
  
  }else if (humid < UMID_THRESH && digitalRead(PIN_FAN) == HIGH){
    digitalWrite(PIN_FAN, LOW);
    fan_sleep_counter = 0;
  }

  
}

void show_data_temp_humid(float t, float h){
    if (isnan(h) || isnan(t))
  {
    Serial.println("Falha DHT!");
  }
  
  if (isnan(t)){
    lcd.setCursor(6,0);
    lcd.print("fail     ");
  }else{
    lcd.setCursor(6,0);
    lcd.print(t);
    lcd.setCursor(10,0);
    lcd.print(" ");
  }
  
  if (isnan(h)){
    lcd.setCursor(6,1);
    lcd.print("fail     ");
  }else{
    lcd.setCursor(6,1);
    lcd.print(h);
    lcd.setCursor(10,1);
    lcd.print(" ");
  }
}


void setup_lcd_t_u(){
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
  if (data_file){
      data_file.println(timestamp);
      data_file.println(",");
      data_file.println(temp);
      data_file.println(",");
      data_file.println(humid);
      
      Serial.println("SD OK!");
      lcd.setCursor(15,0);
      lcd.print("o");
  } else{
      Serial.println("SD ERRO!");
      lcd.setCursor(15,0);
      lcd.print("f");
  }
  data_file.close();
}

String timeRTC(){
  
  // Le os valores (data e hora) do modulo DS1307
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS, 7);
  int segundos = ConverteparaDecimal(Wire.read());
  int minutos = ConverteparaDecimal(Wire.read());
  int horas = ConverteparaDecimal(Wire.read() & 0b111111);
  int diadasemana = ConverteparaDecimal(Wire.read()); 
  int diadomes = ConverteparaDecimal(Wire.read());
  int mes = ConverteparaDecimal(Wire.read());
  int ano = ConverteparaDecimal(Wire.read());

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

void SelecionaDataeHora()   //Seta a data e a hora do DS1307
{
  byte segundos = 10; //Valores de 0 a 59
  byte minutos = 01; //Valores de 0 a 59
  byte horas = 17; //Valores de 0 a 23
  byte diadasemana = 0; //Valores de 0 a 6 - 0=Domingo, 1 = Segunda, etc.
  byte diadomes = 12; //Valores de 1 a 31
  byte mes = 2; //Valores de 1 a 12
  byte ano = 17; //Valores de 0 a 99
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); //Stop no CI para que o mesmo possa receber os dados

  //As linhas abaixo escrevem no CI os valores de 
  //data e hora que foram colocados nas variaveis acima
  Wire.write(ConverteParaBCD(segundos));
  Wire.write(ConverteParaBCD(minutos));
  Wire.write(ConverteParaBCD(horas));
  Wire.write(ConverteParaBCD(diadasemana));
  Wire.write(ConverteParaBCD(diadomes));
  Wire.write(ConverteParaBCD(mes));
  Wire.write(ConverteParaBCD(ano));
  Wire.write(zero);
  Wire.endTransmission(); 
}

byte ConverteParaBCD(byte val)
{ 
  //Converte o número de decimal para BCD
  return ( (val/10*16) + (val%10) );
}

byte ConverteparaDecimal(byte val)  
{ 
  //Converte de BCD para decimal
  return ( (val/16*10) + (val%16) );
}
