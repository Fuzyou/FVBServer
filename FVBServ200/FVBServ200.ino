#include <xbee.h>
#include <Stewitter.h>
#include <SoftwareSerial.h>

/* Arduino Forced Ventilation Box Server

	 FVB Server Version 1.01 add NTP
	 2014/6/29
 */



/* NTP Setup*/
#include<SPI.h>
#include<Ethernet.h>
#include<EthernetUdp.h>
#include<Time.h>
#include <string.h>

byte mac[] = {0x90,0xA2,0xDA,0x00,0x3B,0x36};
unsigned int localPort = 8888;
const int NTP_PACKET_SIZE = 48;
unsigned long lastSendPacletTime = 0;
IPAddress timeServer(210,173,160,27); 
byte packetBuffer[NTP_PACKET_SIZE];
EthernetUDP Udp;
unsigned long lastSendPacketTime = 0;
const unsigned int TW_BUFFER_SIZE = 128;
char timeStr[TW_BUFFER_SIZE];
char dataStr[384];

Stewitter twitter("f9a13cdffca05042a00a5f5d9ae39b22");

SoftwareSerial Xbee(3,2);

byte dev_gpio_A[] = {0x00,0x13,0xA2,0x00,0x40,0x64,0xF1,0x40};
byte dev_gpio_B[] = {0x00,0x13,0xA2,0x00,0x40,0x86,0xB2,0x46};

double temp_A; 
double rh_A;
double temp_B;
double rh_B;

int	seq = 3000;


void setup(){

		Serial.begin(9600);
		Xbee.begin(9600);	

		while(Ethernet.begin(mac) == 0) {
				Serial.print("Configuring Ethernet using DHCP..");
				delay(10000);	
		}

		Serial.print("My IP address is ");
		Serial.println(Ethernet.localIP());



		Udp.begin(localPort);

		sendNTPpacket(timeServer);
		lastSendPacketTime = millis();
		xbee_init(0x00);

		delay(500);
}

void get_data(){

		temp_A = xbee_adc(dev_gpio_A , 1 );
		rh_A = xbee_adc(dev_gpio_A , 2);
		temp_B = xbee_adc(dev_gpio_B , 1 );
		rh_B = xbee_adc(dev_gpio_B , 2);
}

void cal_data(){
		temp_A = temp_A / 1024 * 1.2 * 100-60;
		rh_A = rh_A / 1024 * 1.2 * 100;
		temp_B = temp_B / 1024 * 1.2 * 100-60;
		rh_B = rh_B / 1024 * 1.2 * 100;
}


void debug(){
		Serial.print(temp_A);
		Serial.print(",");
		Serial.println(rh_A);
		Serial.print(temp_B);
		Serial.print(",");
		Serial.println(rh_B);
}

void fan_ON(){

		xbee_gpo(dev_gpio_A , 3 , 1 );
}

void makeXbeeSleep(){
		xbee_end_device(dev_gpio_A, 28, 0, 0);
		xbee_end_device(dev_gpio_B, 28, 0, 0);
}


void makeXbeeWakeup(){
		xbee_rat(dev_gpio_A, "ATSM00");
		xbee_rat(dev_gpio_B, "ATSM00");
}


void loop(){

		temp_A = 0;
		temp_B = 0;
		rh_A = 0;
		rh_B = 0;

		getTime();

		int	time_now = minute() ;

		if( 6 == time_now ){ 
				
				makeXbeeSleep();

				Serial.println("Make Xbee sleep");
		}
		else if( 0 <= time_now && time_now <= 5)
		{
				makeXbeeWakeup();

				Serial.println("Xbee wake UP");

				fan_ON();

				memset(timeStr,0,sizeof(timeStr));
				memset(dataStr,0,sizeof(dataStr));

				makeTimeStr(timeStr);
				get_data();
				makeDataStr(dataStr);
				strcat(dataStr,timeStr);
				Serial.println(dataStr);
				//(twitter.post(dataStr));
		}else{
				Serial.println("Xbee are sleeping");
		}
		debug();
		delay(seq);
}

unsigned long getTime(){



		if ( Udp.parsePacket() ) {
				Udp.read(packetBuffer, NTP_PACKET_SIZE);

				unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
				unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

				unsigned long secsSince1900 = highWord << 16 | lowWord;
				Serial.print("Seconds since Jan 1 1900 = " );
				Serial.println(secsSince1900);

				const unsigned long seventyYears = 2208988800UL;
				unsigned long epoch = secsSince1900 - seventyYears;  
				Serial.print("Unix time = ");
				Serial.println(epoch);

				setTime(epoch + (9 * 60 * 60));
		}
}

void makeTimeStr(char *str) {
		char tmp[16];

		sprintf(tmp, "%04d/", year());
		strcpy(str, tmp);

		sprintf(tmp, "%02d/", month());
		strcat(str, tmp);

		sprintf(tmp, "%02d ", day());
		strcat(str, tmp);

		sprintf(tmp, "%02d:", hour());
		strcat(str, tmp);

		sprintf(tmp, "%02d", minute());
		strcat(str, tmp);
}

unsigned long sendNTPpacket(IPAddress& address){

		memset(packetBuffer, 0, NTP_PACKET_SIZE); 
		packetBuffer[0] = 0b11100011;   // LI, Version, Mode
		packetBuffer[1] = 0;     // Stratum, or type of clock
		packetBuffer[2] = 6;     // Polling Interval
		packetBuffer[3] = 0xEC;  // Peer Clock Precision
		packetBuffer[12]  = 49; 
		packetBuffer[13]  = 0x4E;
		packetBuffer[14]  = 49;
		packetBuffer[15]  = 52;
		Udp.beginPacket(address, 123);
		Udp.write(packetBuffer, NTP_PACKET_SIZE);
		Udp.endPacket();
}

void makeDataStr(char *str) {
		char tmp[64] ={0};

		memset(tmp, 0, sizeof(tmp));

		sprintf(tmp, "%s,", "A");
		strcpy(str, tmp);

		memset(tmp, 0, sizeof(tmp));

		dtostrf(temp_A,-1,2,tmp);
		strcat(str, tmp);

		sprintf(tmp,"%s",",");
		strcat(str, tmp);

		memset(tmp, 0, sizeof(tmp));

		dtostrf(rh_A,-1,2,tmp);
		strcat(str, tmp);

		sprintf(tmp,"%s",",");
		strcat(str, tmp);
}

