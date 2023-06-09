#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <stdint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2lib.h>
#include <OneButton.h>

const int LED1 = 12;
const int LED2 = 13;

#define OLED_SDA 4
#define OLED_SCL 5
#define OLED_RST -1

bool ledState = LOW;
int pinAnterior = -1;
bool estadosLED[8] = {false, false, false, false, false, false, false, false};
int pinesRelay[] = {0, 1, 12, 13, 2, 3, 10, 6};


// Credenciales WiFi
const char *ssid = "CLARO-B612-9B21";
const char *pass = "3nDRaqQDFD";
// Credenciales MQTT
const char *brokerUser = "dev1";
const char *brokerPass = "pass";
const char *broker = "192.168.1.109";

WiFiClient espClient;
PubSubClient client(espClient);

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/OLED_SCL, /* data=*/OLED_SDA, /* reset=*/OLED_RST);

const int NUM_ITEMS = 8;        // number of items in the list and also the number of screenshots and screenshots with QR codes (other screens)
const int MAX_ITEM_LENGTH = 20; // maximum characters for the item name

#define BUTTON 7   // pin for UP button

int button_up_clicked = 0;     // only perform action when button is clicked, and wait until another press
int button_select_clicked = 0; // same as above
int button_down_clicked = 0;   // same as above

int item_selected = 0; // which item in the menu is selected

int item_sel_previous; // previous item - used in the menu screen to draw the item before the selected one
int item_sel_next;     // next item - used in the menu screen to draw next item after the selected one

int current_screen = 0; // 0 = menu, 1 = screenshot, 2 = qr

OneButton btn = OneButton(
  BUTTON,  // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);


// 'scrollbar_background', 8x64px
const unsigned char bitmap_scrollbar_background [] PROGMEM = {
  0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 
  0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 
  0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 
  0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 
  0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 
  0x00, 0x40, 0x00, 0x00, };


// 'item_sel_outline', 128x21px
const unsigned char bitmap_item_sel_outline [] PROGMEM = {
  0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0C, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0xF8, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 
  };

// 'icon_3dcube', 16x16px
const unsigned char bitmap_icon_luz [] PROGMEM = {
	0x00, 0x00, 0xf0, 0x0f, 0x08, 0x10, 0x08, 0x10, 0x04, 0x20, 0x44, 0x22, 0x44, 0x22, 0x44, 0x22, 
	0x48, 0x12, 0x48, 0x12, 0x50, 0x0a, 0x50, 0x0a, 0xe0, 0x07, 0xe0, 0x07, 0xc0, 0x03, 0x00, 0x00
};
// 'icon_battery', 16x16px
const unsigned char bitmap_icon_battery [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x1F, 0x02, 0x20, 
  0xDA, 0x66, 0xDA, 0x66, 0xDA, 0x66, 0x02, 0x20, 0xFC, 0x1F, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
// 'icon_dashboard', 16x16px
const unsigned char bitmap_icon_dashboard [] PROGMEM = {
  0xE0, 0x07, 0x18, 0x18, 0x84, 0x24, 0x0A, 0x40, 0x12, 0x50, 0x21, 0x80, 
  0xC1, 0x81, 0x45, 0xA2, 0x41, 0x82, 0x81, 0x81, 0x05, 0xA0, 0x02, 0x40, 
  0xD2, 0x4B, 0xC4, 0x23, 0x18, 0x18, 0xE0, 0x07, };
// 'icon_fireworks', 16x16px
const unsigned char bitmap_icon_fireworks [] PROGMEM = {
  0x00, 0x00, 0x00, 0x10, 0x00, 0x29, 0x08, 0x10, 0x08, 0x00, 0x36, 0x00, 
  0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x08, 
  0x20, 0x08, 0x50, 0x00, 0x20, 0x00, 0x00, 0x00, };
// 'icon_gps_speed', 16x16px
const unsigned char bitmap_icon_gps_speed [] PROGMEM = {
  0x00, 0x00, 0xC0, 0x0F, 0x00, 0x10, 0x80, 0x27, 0x00, 0x48, 0x00, 0x53, 
  0x60, 0x54, 0xE0, 0x54, 0xE0, 0x51, 0xE0, 0x43, 0xE0, 0x03, 0x50, 0x00, 
  0xF8, 0x00, 0x04, 0x01, 0xFE, 0x03, 0x00, 0x00, };
// 'icon_knob_over_oled', 16x16px
const unsigned char bitmap_icon_knob_over_oled [] PROGMEM = {
  0x00, 0x00, 0xF8, 0x0F, 0xC8, 0x0A, 0xD8, 0x0D, 0x88, 0x0A, 0xF8, 0x0F, 
  0xC0, 0x01, 0x80, 0x00, 0x00, 0x00, 0x90, 0x04, 0x92, 0x24, 0x04, 0x10, 
  0x00, 0x80, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, };
// 'icon_parksensor', 16x16px
const unsigned char bitmap_icon_parksensor [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x44, 0x00, 0xA4, 0x00, 
  0x9F, 0x00, 0x00, 0x81, 0x30, 0xA1, 0x48, 0xA9, 0x4B, 0xA9, 0x30, 0xA0, 
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
// 'icon_turbo', 16x16px
const unsigned char bitmap_icon_turbo [] PROGMEM = {
  0x00, 0x70, 0xE0, 0x8F, 0x18, 0x80, 0x04, 0x80, 0x02, 0x80, 0xC2, 0x8F, 
  0x21, 0x72, 0x51, 0x05, 0x91, 0x44, 0x51, 0x45, 0x21, 0x42, 0xC2, 0x21, 
  0x02, 0x20, 0x04, 0x10, 0x18, 0x0C, 0xE0, 0x03, };

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 384)
const unsigned char* bitmap_icons[8] = {
  bitmap_icon_luz,
  bitmap_icon_luz,
  bitmap_icon_luz,
  bitmap_icon_luz,
  bitmap_icon_luz,
  bitmap_icon_luz,
  bitmap_icon_luz,
  bitmap_icon_luz
};

char menu_items[NUM_ITEMS][MAX_ITEM_LENGTH] = { // array with item names
    {"Luz Josue"},
    {"Luz Edu"},
    {"Luz Papas"},
    {"Luz Patio1"},
    {"Luz Baño"},
    {"Luz Patio2"},
    {"Luz Cruz"},
    {"Alarma"}};


// note - when changing the order of items above, make sure the other arrays referencing bitmaps
// also have the same order, for example array "bitmap_icons" for icons, and other arrays for screenshots and QR codes




void setupWifi()
{
  delay(100);
  Serial.println("\nConectando a");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print("-");
  }
  Serial.print("\nConectado a");
  Serial.println(ssid);
  // digitalWrite(LED2, HIGH);
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("\nConectando a");
    Serial.print(broker);
    if (client.connect("xxx", brokerUser, brokerPass))
    {
      Serial.print("\nConectado a");
      Serial.print(broker);
      // digitalWrite(LED1, HIGH);
    }
    else
    {
      Serial.println("\nIntentando conectar denuevo");
      delay(5000);
    }
  }
}

void button_up() {
  // digitalWrite(LED1, HIGH);
  item_selected = item_selected - 1; // select previous item
      // button_up_clicked = 1;             // set button to clicked to only perform the action once
      if (item_selected < 0)
      { // if first item was selected, jump to last item
        item_selected = NUM_ITEMS - 1;
      }
  //     delay(100);
  // digitalWrite(LED1, LOW);
}

void button_down(){
  // digitalWrite(LED2, HIGH);
  item_selected = item_selected + 1; // select next item
      // button_down_clicked = 1;           // set button to clicked to only perform the action once
      if (item_selected >= NUM_ITEMS)
      { // last item was selected, jump to first menu item
        item_selected = 0;
      }
  //     delay(100);

  // digitalWrite(LED2, LOW);
}

void button_press(){
  char* valorBuscado = menu_items[item_selected];
  int longitudArray = sizeof(menu_items) / sizeof(menu_items[0]);

  int posicion = -1; // Inicializa la variable de posición en -1 (valor predeterminado si no se encuentra el valor)

  // Recorre el array buscando el valor
  for (int i = 0; i < longitudArray; i++) {
  if (strcmp(menu_items[i], valorBuscado) == 0) {
    posicion = i; // Guarda la posición si se encuentra el valor
    break; // Rompe el bucle una vez se haya encontrado el valor
  }
}

  if (posicion != -1) {
  Serial.print("El valor '");
  Serial.print(valorBuscado);
  Serial.print("' se encuentra en la posición ");
  Serial.println(posicion);

  // Verifica si la posición es válida para acceder al array de pines
  if (posicion < sizeof(pinesRelay) / sizeof(pinesRelay[0])) {
    int pinActual = pinesRelay[posicion];
        
    int posicionAnterior = (posicion == 0) ? (sizeof(estadosLED) / sizeof(estadosLED[0])) - 1 : posicion - 1;
      

      // Cambia el estado del LED actual
      estadosLED[posicion] = !estadosLED[posicion];
      digitalWrite(pinActual, estadosLED[posicion] ? HIGH : LOW);

    delay(100);
    u8g2.clearBuffer(); 
    u8g2.setCursor(45,30); 
    u8g2.setFont(u8g_font_7x14);
    u8g2.print(menu_items[posicion]);
    u8g2.setCursor(60,60); 
    u8g2.print(estadosLED[posicion]);
    u8g2.sendBuffer();
    delay(500);
    // Aquí puedes agregar el código que deseas ejecutar con el número de pin
    // Utiliza la variable 'pin' para referirte al número del pin correspondiente
    // Por ejemplo, puedes encender o apagar un LED conectado a ese pin
    // Ejemplo: digitalWrite(pin, HIGH);
  } else {
    Serial.println("La posición del valor no es válida para acceder al array de pines.");
  }
} else {
  Serial.print("El valor '");
  Serial.print(valorBuscado);
  Serial.println("' no se encontró en el array de valores.");
}
}



void setup()
{
  u8g2.setColorIndex(1); // set the color to white
  u8g2.begin();
  u8g2.setBitmapMode(1);

  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  // pinMode(12, OUTPUT);
  // pinMode(13, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(6, OUTPUT);


  // btn.setDebounceTicks(80);
  // btn.setClickTicks(600);
  btn.setPressTicks(1000);

  btn.attachClick(button_down);
  btn.attachDoubleClick(button_up);
  btn.attachDuringLongPress(button_press);


  setupWifi();
  client.setServer(broker, 1883);

}

void menuDisplay(){
  if (current_screen == 0)
  { // MENU SCREEN
    

    // selected item background
    u8g2.drawXBMP(0, 22, 128, 21, bitmap_item_sel_outline);

    // draw previous item as icon + label
    u8g2.setFont(u8g_font_7x14);
    u8g2.drawStr(25, 15, menu_items[item_sel_previous]);
    u8g2.drawXBMP(4, 2, 16, 16, bitmap_icons[item_sel_previous]);

    // draw selected item as icon + label in bold font
    u8g2.setFont(u8g_font_7x14B);
    u8g2.drawStr(25, 15 + 20 + 2, menu_items[item_selected]);
    u8g2.drawXBMP(4, 24, 16, 16, bitmap_icons[item_selected]);

    // draw next item as icon + label
    u8g2.setFont(u8g_font_7x14);
    u8g2.drawStr(25, 15 + 20 + 20 + 2 + 2, menu_items[item_sel_next]);
    u8g2.drawXBMP(4, 46, 16, 16, bitmap_icons[item_sel_next]);

    // draw scrollbar background
    u8g2.drawXBMP(128 - 8, 0, 8, 64, bitmap_scrollbar_background);

    // draw scrollbar handle
    u8g2.drawBox(125, 64 / NUM_ITEMS * item_selected, 3, 64 / NUM_ITEMS);

    
  }
}

void loop()
{
  btn.tick();
  delay(10);
  u8g2.clearBuffer(); 
  menuDisplay();
  u8g2.sendBuffer();

   // send buffer from RAM to display controller
  
  // else if (current_screen == 1)
  // {                                                                  // SCREENSHOTS SCREEN
  //   u8g2.drawXBMP(0, 0, 128, 64, bitmap_screenshots[item_selected]); // draw screenshot
  // }
  // else if (current_screen == 2)
  // {                                                               // QR SCREEN
  //   u8g2.drawXBMP(0, 0, 128, 64, bitmap_qr_codes[item_selected]); // draw qr code screenshot
  // }

  //if (current_screen == 0)
   // MENU SCREEN
    
    // up and down buttons only work for the menu screen
    // if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 0))
    // {                                    // up button clicked - jump to previous menu item
    //   item_selected = item_selected - 1; // select previous item
    //   button_up_clicked = 1;             // set button to clicked to only perform the action once
    //   if (item_selected < 0)
    //   { // if first item was selected, jump to last item
    //     item_selected = NUM_ITEMS - 1;
    //   }
    // }
    // else if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 0))
    // {                                    // down button clicked - jump to next menu item
    //   item_selected = item_selected + 1; // select next item
    //   button_down_clicked = 1;           // set button to clicked to only perform the action once
    //   if (item_selected >= NUM_ITEMS)
    //   { // last item was selected, jump to first menu item
    //     item_selected = 0;
    //   }
    // }

    // if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1))
    // { // unclick
    //   button_up_clicked = 0;
    // }
    // if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1))
    // { // unclick
    //   button_down_clicked = 0;
    // }
  

  // if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0))
  // {                            // select button clicked, jump between screens
  //   button_select_clicked = 1; // set button to clicked to only perform the action once
  //   if (current_screen == 0)
  //   {
  //     current_screen = 1;
  //   } // menu items screen --> screenshots screen
  //   else
  //   {
  //     current_screen = 0;
  //   } // qr codes screen --> menu items screen
  // }
  // if ((digitalRead(BUTTON_SELECT_PIN) == HIGH) && (button_select_clicked == 1))
  // { // unclick
  //   button_select_clicked = 0;
  // }

  // set correct values for the previous and next items
  item_sel_previous = item_selected - 1;
  if (item_sel_previous < 0)
  {
    item_sel_previous = NUM_ITEMS - 1;
  } // previous item would be below first = make it the last
  item_sel_next = item_selected + 1;
  if (item_sel_next >= NUM_ITEMS)
  {
    item_sel_next = 0;
  } // next item would be after last = make it the first

  

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}
