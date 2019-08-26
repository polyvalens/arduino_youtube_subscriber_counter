/*
 Arduino sketch for WIZnet IoT iOffload Contest 2019
 
 A simple web server that shows the values of two parameters
 received by means of a GET request on an LCD and on an HTML page.
 
 Circuit:
 - WIZ610io module attached to pins 3 (SS, see notes below), 11, 12, 13
 - LCD attached to pins 4-10 (using "LCD Keypad Shield")

 Based on Arduino example Ethernet->WebServer

 Notes:
 - Requires replacing of default Arduino Ethernet library by
   Ethernet-W6100 library from WIZnet.
 - After replacing, modify libraries\Ethernet\src\utility\w5100.cpp, line 36:
   #define SS_PIN_DEFAULT  3

 Tested on Arduino 1.8.5

 Copyright (C) 2019 Clemens Valens

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
 
#include <LiquidCrystal.h>

const int rs = 8;
const int en = 9;
const int d4 = 4;
const int d5 = 5;
const int d6 = 6;
const int d7 = 7;
const int backlight = 10;
LiquidCrystal lcd(rs,en,d4,d5,d6,d7);

// LCD Keypad Shield key approximate voltage levels.
const int keys = A0; // Resistor ladder on A0.
const int key_select = 724; // 724
const int key_left = 485; // 485
const int key_down = 308; // 308
const int key_up = 133; // 133
const int key_right = 0; // 0

#include <SPI.h>
// Stupid Ethernet library assumes SS pin is at pin 10. Move it to pin 3 
// because LCD Keypad Shield uses pin 10 for backlight control. Make this
// mod in the library as there does not seem to be an easy other way :-(
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] =
{
  0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed
};
IPAddress ip(192,168,1,177);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

#define RX_BUFFER_SIZE  (64)
char rx_buffer[RX_BUFFER_SIZE];
uint8_t rx_buffer_index = 0;

#define PARAMETER_SIZE  (32)
char parameter1[PARAMETER_SIZE];
char parameter2[PARAMETER_SIZE];

bool is_get(void)
{
  return (rx_buffer[0]=='G' && rx_buffer[1]=='E' && rx_buffer[2]=='T');
}

#define SEPARATOR  ('"')

int8_t to_hex(char ch)
{
  if (ch>='0' && ch<='9') return ch-'0';
  if (ch>='A' && ch<='F') return ch-'A';
  return -1;
}

char url_convert_percent_char(char *p_src, uint8_t *p_result)
{
  char ch = p_src[0];
  *p_result = 1;
  if (p_src[0]=='%')
  {
    int8_t temp = to_hex(p_src[1]);
    if (temp==-1) return p_src[0];
    ch = temp<<4;
    temp = to_hex(p_src[2]);
    if (temp==-1) return p_src[0];
    ch |= temp;
    *p_result = 3;
  }
  return ch;
}

void clean_string(char *p_dst, char *p_src)
{
  while (*p_src!=0)
  {
    uint8_t i = 0;
    *p_dst = url_convert_percent_char(p_src,&i);
    p_dst++;
    p_src += i;
  }
  *p_dst = 0;
}

char *extract_field(char *p_dst, char *p_src)
{
  // Find start of field.
  while (*p_src!=0 && *p_src!=SEPARATOR) p_src++;
  if (*p_src==0) return NULL;
  p_src++; // Skip separator.
  // Copy until end of field.
  while (*p_src!=0 && *p_src!=SEPARATOR)
  {
    // Convert %20 to space.
    //if (*p_src=='%' && *(p_src+1)=='2' && *(p_src+2)=='0')
    //{
    //  *p_dst = ' '; // Replace char by space.
    //  p_src += 2;
    //}
    *p_dst = *p_src; // Copy char.
    p_dst++;
    p_src++;
  }
  *p_dst = 0;
  p_src++;
  return p_src;
}

void parse_get(char *p_str)
{
  char *p = p_str;
  clean_string(p_str,p_str);
  p = extract_field(parameter1,p);
  p = extract_field(parameter2,p);
}

void show_license(void)
{
  Serial.println("\nWIZnet IoT iOffload Contest 2019");
  Serial.println("Copyright (C) 2019 Clemens Valens");
  Serial.println("This program comes with ABSOLUTELY NO WARRANTY.");
  Serial.println("This is free software, and you are welcome to redistribute it");
  Serial.println("under certain conditions; see source code for details.\n");
}

void setup(void)
{
  pinMode(backlight,OUTPUT);
  digitalWrite(backlight,1);
  lcd.begin(16,2);
  //         0123456789012345
  lcd.print("WIZnet IoT");
  lcd.setCursor(0,1);
  lcd.print("iOffload Contest");
  delay(1000);
  
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  show_license();

  // Start the Ethernet connection and the server:
  Ethernet.begin(mac,ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus()==EthernetNoHardware)
  {
    Serial.println("WIZ610io module not found.");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus()==LinkOFF)
  {
    Serial.println("Ethernet cable is not connected.");
  }

  // Start the server
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());

  lcd.clear();
  lcd.print("My IP:");
  lcd.setCursor(0,1);
  lcd.print(Ethernet.localIP());
  delay(1000);
}

void loop(void)
{
  // Listen for incoming clients.
  EthernetClient client = server.available();
  if (client)
  {
    Serial.print(millis());
    Serial.println(": new client");
    // An HTTP request ends with a blank line.
    rx_buffer_index = 0;
    bool currentLineIsBlank = true;
    while (client.connected())
    {
      if (client.available())
      {
        char ch = client.read();

        if (rx_buffer_index<RX_BUFFER_SIZE-1)
        {
          rx_buffer[rx_buffer_index] = ch;
          rx_buffer_index += 1;
          rx_buffer[rx_buffer_index] = 0; // Zero terminate.
        }
        
        //Serial.write(ch);
        // If you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply.
        if (ch =='\n' && currentLineIsBlank)
        {
          if (is_get()==true)
          {
            parse_get(rx_buffer);
          }
          else Serial.println("No GET.");
          // Send a standard HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // The connection will be closed after completion of the response.
          //client.println("Refresh: 5");  // Refresh the page automatically every 5 seconds.
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print("Channel: ");
          client.print(parameter1);
          client.println("<br />");
          client.print("Subscribers: ");
          client.print(parameter2);
          client.println("<br />");
          client.println("</html>");

          lcd.clear();
          parameter1[16] = 0;
          lcd.print(parameter1);
          lcd.setCursor(0,1);
          parameter2[16] = 0;
          lcd.print(parameter2);

          break;
        }
        if (ch == '\n')
        {
          // You're starting a new line.
          currentLineIsBlank = true;
        }
        else if (ch != '\r')
        {
          // You've gotten a character on the current line.
          currentLineIsBlank = false;
        }
      }
    }
    // Give the web browser time to receive the data
    delay(1);
    // Close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

