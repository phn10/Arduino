#include <SPI.h>
#define CE_PIN 4        // CE pin used to send data or receive data btw two microcontroller
#define CSN_PIN 5       // CSN pin used to enable SPI 
#define IRQ_PIN 2 
#define MOSI_PIN 11
#define MISO_PIN 12
#define SCK_PIN 13

//global variable used by everyone
byte data_in[5], data2, data3;

void setup()
{
  Serial.begin(115200);
  Serial.println("Setting up");
  delay(100);
  NRF_Init();
  NRF_set_RX_payload(0, 3);
  NRF_get_address(7, 1);         // get status register # 7, 1 = print screen
  NRFwrite_bit_write(0, 0, 1);   // register #, bit #, and value 0 or 1: RX mode
  NRFwrite_bit_write(0, 1, 1);   // register #, bit #, and value 0 or 1: Power Up
  NRFwrite_bit_write(0, 4, 1);   // RT Mask turns off the RT interrupt
  NRFwrite_bit_write(0, 5, 1);   // TX Mask turns off the TX interrupt

  // pull the CSN pin to low to active the SPI to receive data
  digitalWrite(CSN_PIN, LOW);  
  data_in[0] = SPI.transfer(B11100010); // flush RX
  digitalWrite(CSN_PIN,  HIGH);
  digitalWrite(CSN_PIN, LOW);
  data_in[0] = SPI.transfer(B11100001); // flush TX
  digitalWrite(CSN_PIN, HIGH);

  NRF_ClearInterrupts();
  delay(100);
  attachInterrupt(0, get_data, FALLING); 

  NRFpinMode(3, 0);   // pinmode routine :: pin #, value: 0 = LOW, 1 = HIGH
  NRFpinWrite(3, 1);  // pinwrite routine :: pin #, value: 0 = LOW, 1 = HIGH
}

void loop()
{
  NRF_ping();
  Serial.println(NRFpinRead(3));  //

  if (NRFpinRead(3) == 0)
  {
    NRFpinMode(7, 1); // pin 7 as OUTPUT
    NRFpinWrite(7, 1); // pin 7 as HIGH
    delay(100);
    NRFpinWrite(7, 0); // pin 7 as LOW
    delay(100);
  }
}

/** transmit data from host micro controller to slave micro controller
  * @param mode unknow
  * @param pin the pin of slave micro controller
  * @param value the value of that pin
  */
void transmit(byte mode, byte pin, byte value)
{
  // pull CSN pin to low to talk  to the SPI
  digitalWrite(CSN_PIN, LOW);
  // instruction W_TX_payload :: write to the payload 
  data_in[0] = SPI.transfer(B10100000); 
  // write 3 byte in TX payload
  data_in[1] = SPI.transfer(mode);
  data_in[2] = SPI.transfer(pin);
  data_in[3] = SPI.transfer(value);
  // put CSN pin to high to finish data transmission
  digitalWrite(CSN_PIN, HIGH);

  digitalWrite(CE_PIN, LOW);   // pull CE pin to low 
  delay(1);
  NRFwrite_bit_write(0, 0, 0); // change SPI to TX mode
  delay(1);
  digitalWrite(CE_PIN, HIGH);  // pull CE pin to high to send data to slave micro controller
  delay(1);
  NRFwrite_bit_write(0, 0, 1); // go back to RX mode

  // both micro controller are in RX mode, that why they need to enable CE pin to HIGH
  // to always listen to signal from other devices. When we transmit data from master
  // micro controller to slave micro controller, we have to do 2 things: 
  // 1. pull CE pin to LOW
  // 2. change the SPI to TX mode
  // 3. pull CE pin to HIGH again to send data
  // 4. change the SPI back to RX mode
}

/** get the data from the RX payload
  */
void get_data()
{
  int i;
  digitalWrite(CSN_PIN, LOW);      // pull CSN pin to LOW to talk to the SPI
  data_in[0] = SPI.transfer(B01100001);  // read payload
  data_in[1] = SPI.transfer(B00000000);  // get the mode 
  data_in[2] = SPI.transfer(B00000000);  // get the pin
  data_in[3] = SPI.transfer(B00000000);  // get the value 
  digitalWrite(CSN_PIN, HIGH);     // pull CSN pin to HIGH to stop takling

  if (data_in[1] == 1)  // data starting with 1 sets up the pin mode
  {
    if (data_in[3] == 0)
      pinMode(data_in[2], INPUT); 
    if (data_in[3] == 1)
      pinMode(data_in[2], OUTPUT);
    delay(10);
    transmit(3, data_in[2], data_in[3]); // send data back for verification
  }
  else if (data_in[1] == 2)  // data starting with 2 sets writes to the pin
  {
    if (data_in[3] == 0)
      digitalWrite(data_in[2], LOW);
    if (data_in[3] == 1)
      digitalWrite(data_in[2], HIGH);
    delay(10);
    transmit(3, data_in[2], data_in[3]); // send back for verification
  }
  else if (data_in[1] == 3)  // echo back used to verift the right data was sent
  {
    data2 = data_in[2];
    data3 = data_in[3];
  }
  else if (data_in[1] == 4)  // digital read
  {
    delay(10);
    transmit(3, data_in[2], digitalRead(data_in[2]));
  }
  else if (data_in[1] == 5)  // analog read
  {
    delay (10);
    transmit(3, data_in[2], analogRead(data_in[2]));
  }
  else if (data_in[1] == 6)  // ping transmit
  {
    delay(10);
    transmit(3, data_in[2], data_in[3]);
  }
  else if (data_in[1] == 7) // not implemented yet  
  {
  }
  else 
  {
    Serial.println("No Mode Byte Identified");
    for (i = 1; i < 4; i++)
      Serial.print(char(data_in[i]));
    Serial.println(" ");
  }

  digitalWrite(CSN_PIN, LOW);
  data_in[0] = SPI.transfer(B11100010);
  digitalWrite(CSN_PIN, HIGH);

  NRFwrite_bit_write(7, 6, 1);
}

/** setup basic pin mode of the arduino and start up SPI library
  */
void NRF_Init()
{
  pinMode(CE_PIN, OUTPUT);   // chip enable as output
  pinMode(CSN_PIN, OUTPUT);  // chip select not as output
  pinMode(MOSI_PIN, OUTPUT); // SPI data out
  pinMode(MISO_PIN, INPUT);  // SPI data in
  pinMode(SCK_PIN, OUTPUT);  // SPI clock out

  Serial.println("NRF Pin Initialized");

  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);  // Mode 0: Rising edeg of data, keep clock Low
  SPI.setClockDivider(SPI_CLOCK_DIV2);  // Run the data in at 16MHz / 2 = 8MHz
  digitalWrite(CE_PIN, HIGH);
  digitalWrite(CSN_PIN, HIGH);
  SPI.begin();  // start up the SPI library
  Serial.println("NRF Ready");
}

/** write data to the pipe in the payload
  * @param pipe the pipe # you want to write to, there are 5 pipe in total
  * @param bytes the data you want to send the the pipe, represent in byte
  */
void NRF_set_RX_payload(byte pipe, byte bytes)
{
  byte address = pipe + 32 + 16 + 1;  // a register write start at 32 so add 17, we're writing to register 17
  digitalWrite(CSN_PIN, LOW);
  data_in[0] = SPI.transfer(address);
  data_in[1] = SPI.transfer(bytes);
  digitalWrite(CSN_PIN, HIGH);
  Serial.print("RX Payload Set RX_PW_P");
  Serial.print(pipe);
  Serial.print(" for ");
  Serial.print(bytes);
  Serial.println(" bytes");
}

/** write single bits to  a register without affecting the rest of the register
  * @param address the address of the register you want to write
  * @param bit_add the value of the bit you want to write
  * @param val 
  */
void NRFwrite_bit_write(byte address, byte bit_add, byte val)
{
  NRF_get_address(address, 0);  // read the register without printing it to the screen
  if (val == 1)  // if we want to write a one to the bit then set the bit in the register we read
    bitSet(data_in[1], bit_add);
  else
    bitClear(data_in[1], bit_add);

  digitalWrite(CSN_PIN, LOW);
  data_in[0] = SPI.transfer(32 + address);  // add 32 to the address to write to that register
  data_in[1] = SPI.transfer(data_in[1]);    // write the modified register
  digitalWrite(CSN_PIN, HIGH);
}

/** clear 3 interrupt in the status register, address is 7
  */
void NRF_ClearInterrupts()
{
  NRF_get_address(7, 0);  // check the RT interrupt
  if (bitRead(data_in[1], 4))
    NRFwrite_bit_write(7, 4, 1);
  
  NRF_get_address(7, 0);  // check the TX interrupt
  if (bitRead(data_in[1], 5))
    NRFwrite_bit_write(7, 5, 1);

  NRF_get_address(7, 0);
  if (bitRead(data_in[1], 6))
    NRFwrite_bit_write(7, 6, 1);
} 

/** send a random number to other SPI and receive the feed back
  * if they are the same numbers, ping successful, else ping fail
  */
void NRF_ping()
{
  int ping;
  ping = random(256);  // get a random byte
  Serial.print("Ping with: ");
  Serial.print(ping);

  transmit(6, ping, ping);
  delay(15);
  if (data2 == ping && data3 == ping)
    Serial.println("PING Successful!");
  else
    Serial.println("PING FAIL!");
  data2 = 0;   // reset
}

/** set pinmode of the other micro controller
  * @param pin the pin # of other micro controller
  * @param mode the mode you want to set :: 0 for INPUT, 1 for OUTPUT
  */
void NRFpinMode(byte pin, byte mode)
{
  int i;
  for (i = 0; i < 10; i++)
  {
    transmit(1, pin, mode);
    delay(15);
    if (data2 == pin && mode == data3)
      i = 10;

    if (i = 9)
      Serial.println("Failed to set mode");
  }
  data2 = 0;
  data3 = 0;
}

/**
  */
void NRFpinWrite(byte pin, byte val)
{
  int i;
  for (i = 0; i < 10; i++)
  {
    transmit(2, pin, val);
    delay(15);
    if (data2 == pin && data3 == val)
      i = 10;

    if (i == 9)
      Serial.println("Failed to write pin");
  }
  data2 = 0;
  data3 = 0;
}

/** read value of the pin
  * @param pin the pin # on the other micro controller
  */
byte NRFpinRead(byte pin)
{
  int i;
  for (i = 0; i < 10; i++)
  {
    transmit(4, pin, pin);
    delay(15);
    if (data2 == pin)
    {
      i = 10;
      return (data3);
    }
    
    if (i == 9)
        Serial.println("Failed to read pin");
  }
  data2 = 0;
  data3 = 0;
}

void NRF_get_address(byte address, byte info){// START Get Address   START Get Address   START Get Address  
  //send the address and either a 1 or 0 if you want to do a serial print of the address
  //after a call to this routine, data_in[1] will equal the address you called
  
  digitalWrite(CSN_PIN, LOW);
  data_in[0] = SPI.transfer(address);
  data_in[1] = SPI.transfer(B00000000);
  digitalWrite(CSN_PIN, HIGH);
  if(info==1){// if the user wanted it, you will get a print out of the register - good fo debugging
  Serial.print("R");
  Serial.print(address);
  switch (address) {
  case 0:
   Serial.print(" CONFIG REGISTER =");
   Serial.println(data_in[1]);
   Serial.print("PRIM_RX = ");
   if(bitRead(data_in[1],0))
   Serial.println("PRX");
   else
   Serial.println("PTX");
   
   Serial.print("PWR_UP = ");
   if(bitRead(data_in[1],1))
   Serial.println("POWER UP");
   else
   Serial.println("POWER DOWN");  
   
    Serial.print("CRCO = ");
   if(bitRead(data_in[1],2))
   Serial.println("2Bytes");
   else
   Serial.println("1Byte");   
   
   Serial.print("EN_CRC = ");
   if(bitRead(data_in[1],3))
   Serial.println("Enabled");
   else
   Serial.println("Disabled");  
 
    Serial.print("MASK_MAX_RT = ");
   if(bitRead(data_in[1],4))
   Serial.println("Interrupt not reflected on the IRQ pin");
   else
   Serial.println("Reflect MAX_RT as active low interrupt on the IRQ pin");  
 
   Serial.print("MASK_TX_DS = ");
   if(bitRead(data_in[1],5))
   Serial.println("Interrupt not reflected on the IRQ pin");
   else
   Serial.println("Reflect TX_DS as active low interrupt on the IRQ pin");  
   
   Serial.print("MASK_RX_DR = ");
   if(bitRead(data_in[1],6))
   Serial.println("Interrupt not reflected on the IRQ pin");
   else
   Serial.println("Reflect RX_DR as active low interrupt on the IRQ pin");  
   break;//0
case 1:
   Serial.print(" EN_AA REGISTER Enhanced ShockBurst =");
   Serial.println(data_in[1]);
break;//1
case 2:
   Serial.print(" EN_RXADDR REGISTER Enabled RX Addresses =");
   Serial.println(data_in[1]);
break;//2
case 3:
   Serial.print(" SETUP_AW REGISTER Setup of Address Widths =");
   Serial.println(data_in[1]);
break;//3
case 4:
   Serial.print(" SETUP_RETR REGISTER Setup of Automatic Retransmission =");
   Serial.println(data_in[1]);
break;//4
case 5:
   Serial.print(" RF_CH REGISTER RF Channel =");
   Serial.println(data_in[1]);
break;//5
case 6:
   Serial.print(" RF_SETUP REGISTER RF Setup Register =");
   Serial.println(data_in[1]);
   Serial.print("RF Power = ");
   Serial.print(bitRead(data_in[1],2));
   Serial.println(bitRead(data_in[1],1));
   Serial.print("RF_DR_HIGH = ");
   Serial.println(bitRead(data_in[1],3));
   Serial.print("PLL_LOCK = ");
   Serial.println(bitRead(data_in[1],4));
   Serial.print("RF_DR_LOW = ");
   Serial.println(bitRead(data_in[1],5));  
   Serial.print("CONT_WAVE = ");
   Serial.println(bitRead(data_in[1],7));   
break;//6
case 7:
   Serial.print(" STATUS REGISTER  =");
   Serial.println(data_in[1]);
   Serial.print("TX_FULL = ");
   if(bitRead(data_in[1],0))
   Serial.println("TX FIFO full");
   else
   Serial.println("TX FIFO Not full");
   
   Serial.print("RX_P_NO = ");
   if(bitRead(data_in[1],1)&&(data_in[1],2)&&(data_in[1],3))
   Serial.println("RX FIFO Empty");
   else
   Serial.println(bitRead(data_in[1],1)+(bitRead(data_in[1],2)<<1)+(bitRead(data_in[1],2)<<2)); 
   Serial.print("MAX_RT Interrupt = ");
   Serial.println(bitRead(data_in[1],4));
   Serial.print("TX_DS Interrupt = ");
   Serial.println(bitRead(data_in[1],5));
   Serial.print("RX_DR Interrupt = ");
   Serial.println(bitRead(data_in[1],6));
break;//7
case 8:
   Serial.print(" OBSERVE_TX REGISTER Transmit observe register  =");
   Serial.println(data_in[1]);
   Serial.print("ARC_CNT = "); 
   Serial.println(bitRead(data_in[1],0)+(bitRead(data_in[1],1)<<1)+(bitRead(data_in[1],2)<<2)+(bitRead(data_in[1],3)<<3));
   Serial.print("PLOS_CNT = "); 
   Serial.println(bitRead(data_in[1],4)+(bitRead(data_in[1],5)<<1)+(bitRead(data_in[1],6)<<2)+(bitRead(data_in[1],7)<<3));
break;//8
case 9:
   Serial.print(" RPD REGISTER Received Power Detector =");
   Serial.println(bitRead(data_in[1],0));
break;//9
case 10:
   Serial.print(" RX_ADDR_P0 LSB =");
   Serial.println(data_in[1]);
break;//10
case 11:
   Serial.print(" RX_ADDR_P1 LSB =");
   Serial.println(data_in[1]);
break;//11
case 12:
   Serial.print(" RX_ADDR_P2 LSB =");
   Serial.println(data_in[1]);
break;//12
case 13:
   Serial.print(" RX_ADDR_P3 LSB =");
   Serial.println(data_in[1]);
break;//13
case 14:
   Serial.print(" RX_ADDR_P4 LSB =");
   Serial.println(data_in[1]);
break;//14
case 15:
   Serial.print(" RX_ADDR_P5 LSB =");
   Serial.println(data_in[1]);
break;//15
case 16:
   Serial.print(" TX_ADDR LSB =");
   Serial.println(data_in[1]);
break;//16
case 17:
   Serial.print(" RX_PW_P0 RX payload =");
   Serial.println(data_in[1]);
break;//17
case 18:
   Serial.print(" RX_PW_P1 RX payload =");
   Serial.println(data_in[1]);
break;//18
case 19:
   Serial.print(" RX_PW_P2 RX payload =");
   Serial.println(data_in[1]);
break;//19
case 20:
   Serial.print(" RX_PW_P3 RX payload =");
   Serial.println(data_in[1]);
break;//20
case 21:
   Serial.print(" RX_PW_P4 RX payload =");
   Serial.println(data_in[1]);
break;//21
case 22:
   Serial.print(" RX_PW_P5 RX payload =");
   Serial.println(data_in[1]);
break;//22

case 23:
   Serial.print(" FIFO_STATUS Register =");
   Serial.println(data_in[1]);
   Serial.print("RX_EMPTY = ");
   if(bitRead(data_in[1],0))
   Serial.println("RX FIFO empty");
   else
   Serial.println("Data in RX FIFO");
   
   Serial.print("RX_EMPTY = ");
   if(bitRead(data_in[1],1))
   Serial.println("RX FIFO full");
   else
   Serial.println("Available locations in RX FIFO");
 
    Serial.print("TX_EMPTY = ");
   if(bitRead(data_in[1],4))
   Serial.println("TX FIFO empty");
   else
   Serial.println("Data in TX FIFO");  
  
      Serial.print("TX_FULL = ");
   if(bitRead(data_in[1],5))
   Serial.println("TX FIFO full");
   else
   Serial.println("Available locations in TX FIFO"); 
   Serial.print("TX_REUSE = ");
   Serial.println(bitRead(data_in[1],6));
break;//23
  }//switch
  }
}
