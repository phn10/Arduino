//NRF Arduino
//1 GND GND
//2 VCC 3.3V

#include <SPI.h>
#define CE_pin 4
#define CSN_pin 5
#define IRQ_pin 2
#define MOSI_pin 11
#define MISO_pin 12
#define SCK_pin 13

byte data_in[5], data2, data3;

void setup()
{
  Serial.begin(115200);
  Serial.println("Setting up");
  delay(100);
  NRF_Init(); // setup the SPI and define pins
  NRF_get_RX_payload(0, 3)     // pipe 0-5, bytes 1-32
  NRF_get_address(7, 1)        // get Reg7 Status, 1 = print screen
  NRFwrite_bit_write(0, 0, 1); // register #, bit #, and value 0 or 1
  NRFwrite_bit_write(0, 1, 1);
  NRFwrite_bit_write(0, 4, 1);
  NRFwrite_bit_write(0, 5, 1);
  
  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(B11100010); // flush RX
  digitalWrite(CSN_pin, HIGH);
  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(B11100001);
  digitalWrite(CSN_pin, HIGH);

  NRF_ClearInterrupts(); // clear any interrupts
  delay(100);
  attachInterrupt(0, get_data. FALLING);  // kick things off by attachin the IRQ interrupt

  // control the other arduino
  NRFpinMode(3, 0);        // pinmode routine:: pin number, 0=INPUT, 1=OUTPUT
  NRFpinwrite(3, 1);       // pinmode routune:: pin number, 0=LOW, 1=HIGH
}

void NRF_init()
{
  pinMode(CE_pin, OUTPUT);
  pinMode(CSN_pin, OUTPUT);
  pinMode(MOSI_pin, OUTPUT);
  pinMode(MISO_pin, INPUT);
  pinMode(SCK_pin, OUTPUT);
  Serial.println("NRF Pins Initialized");
  SPI.setBitOrder(MSBFIRST);      // SPI Most Significant Bit First
  SPI.setDataMode(SPI_MODE0);     // Mode 0: Rising edge of data, keep clock low
  SPI.setClockDivider(SPI_CLOCK_DIV2);   // Return the data in at 16MHz / 2 = 8 MHz
  digitalWrite(CE_pin, HIGH);
  digitalWrite(CSN_pin, HIGH);
  SPI.begin();            // start the SPI library
  Serial.println("NRF Ready");
}

void NRF_set_RX_payload(byte pipe, byte bytes)
{
  byte address = pipe + 32 + 16 + 1;
  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(address);    // write register 11 RX_PW_P0
  data_in[1] = SPI.transfer(bytes);
  digitalWrite(CSN_pin, HIGH);
  Serial.println("RX Payload Set RX_PW_P");
  Serial.print(pipe);
  Serial.print(" for ");
  Serial.print(bytes);
  Serial.println(" bytes");
}

void NRFwrite_bit_write(byte address, byte bit_add, byte val)
{
  NRF_get_address(address, 0);

  if (val == 1)
    bitSet(data_in[1], bit_add);
  else
    bitClear(data_in[1], bit_add);
  
  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(32 + address);   // write to a register by adding 32 to it's address
  data_in[1] = SPI.transfer(data_in[1]);     // write the modified register
  
  digitalWrite(CSN_pin, HIGH);
}

// setup pinmode for the other arduino
void NRFpinmode(byte pin, byte mode)
{
  int i;
  for (i = 0; i < 10; i++)  // give it 10 tries to set the pinmode
  {
    transmit(1, pin, mode);  // send out with 1 the pin and the mode
    delay(15);       // give a litte time to respond
    if (data2 == pin && mode == data3)   // if we get the right data back
      i = 10;        // force out of the loop
    if (i == 9)   // if we get too far along, at i == 9, then we never set the pin   
      Serial.println("Failed to set mode");
  }
  data2 = 0;      // reset, what the heck is data1, data2, and data3???
  data3 = 0;
}

// write pin to other arudino
void NRFpinwrite(byte pin, byte val)
{
  int i;
  for (i = 0; i < 10; i++)
  {
    transmit(2, pin, val);
    delay(15);
    if(data2 == pin && val == data3)
      i = 10;
  
    if (i == 9)
      Serial.println("Failed to write pin");
  }
  data2 = 0;
  data3 = 0;
}

// read pins value of other arduino
void NRFpinread(byte pin)
{
  int i;
  for (i = 0; i < 10; i++)
  {
    transmit(4, pin, pin);   // read the transmit and come back here to make sense these!!!!
    delay(15);
    if (data2 == pin)
    {
      i = 10;
      return data3;
    }
    if (i == 9)
      Serial.println("Failed to read pin");
  }
  data2 = 0;
  data3 = 0;
}

// send data to the other arduino
void transmit(byte mode, byte pin, byte value)
{
  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(B11100001); // flush TX, get rid of anything that might be in there
  digitalWrite(CSN_pin, HIGH);

  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(B10100000); // load TX payload
  data_in[1] = SPI.transfer(mode);      // action digital read
  data_in[2] = SPI.transfer(pin);       // pin number
  data_in[3] = SPI.transfer(value);
  digitalWrite(CSN_pin, HIGH);
  
  digitalWrite(CE_pin, LOW); // pull the CE_pin LOW
  delay(1);
  NRFwrite_bit_write(0, 0, 0); // go to TX mode
  delay(1);
  digitalWrite(CE_pin, HIGH);
  delay(1);

  NRFwrite_bit_write(0, 0, 1);
}

  



void NRF_ClearInterrupts()
{
  NRF_get_address(7, 0);          // get the bits in status register

  if (bitRead(data_in[1], 4))     // RT interrupt
    NRFWrite_bit_write(7, 4, 1);
  if (bitRead(data_in[1], 5))     // TX interrupt
    NRFWrite_bit_write(7, 5, 1);
  if (bitRead(data_in[1], 6))     // RX interrupt
    NRFWrite_bit_write(7, 6, 1);
}

void NRF_get_address(byte address, byte info)
{
  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(address);   // data_in[0] is always status register
  data_in[1] = SPI.transfer(B00000000); // data_in[1] is what is inside the register
  digitalWrite(CSN_pin, HIGH); 
  if (info == 1)
  {
    Serial.print("R");
    Serial.print(address);
    switch (address)
    {
      case 0:
        Serial.print(" CONFIG REGISTER = ");      
        Serial.println(data_in[1]);
        
        // bit 0
        Serial.print("PRIM_RX = ");
        if (bitRead(data_in[1], 0))
          Serial.println("PRX");
        else  
          Serial.println("PTX");
        
        // bit 1
        Serial.print("PWR_UP = ");
        if (bitRead(data_in[1], 1))
          Serial.println("POWER UP");
        else
          Serial.println("POWER DOWN");
        
        // bit 2
        Serial.print("CRO = ");
        if (bitRead(data_in[1], 2))
          Serial.println("2Bytes");
        else
          Serial.println("1Bytes");
        
        // bit 3
        Serial.print("EN_CRC = ");
        if (bitRead(data_in[1], 3))
          Serial.println("Enabled");
        else
          Serial.println("Disabled");
        
        // bit 4  
        Serial.print("MASK_MAX_RT = ");
        if (bitRead(data_in[1], 4))
          Serial.println("Interrupt not reflectd on the IRQ pin");
        else
          Serial.println("Reflect MAX_RT as active low interrupt on the IRQ pin");
        
        // pin 5
        Serial.print("MASK_TX_DS = ");
        if (bitRead(data_in[1], 5))
          Serial.println("Interrupt not reflected on the IRQ pin");
        else
          Serial.println("Reflect TX_DS as active low interrupt on the IRQ pin");
        
        // pin 6
        Serial.print("MASK_RX_DR = ");
        if(bitRead(data_in[1], 6))
          Serial.println("Interrupt not reflected on the IRQ pin");
        else
          Serial.println("Reflect RX_DR as active low interrupt on the IRQ pin");
        break;
      
      case 1:
        Serial.print("EN_AA REGISTER Enhaced ShockBust = ");
        Serial.println(data_in[1]);
        break;

      case 2:
        Serial.print("EN_RXADDR REGISTER Enabled RX Address = ");
        Serial.println(data_in[1]);
        break;
      
      case 3:
        Serial.print("SETUP_AW_REGISTER Setup of Address Width = ");
        Serial.println(data_in[1]);
        break;

      case 4:
        Serial.print("SETUP_RETR REGISTER Setup of Automatic Retransmission = ");
        Serial.println(data_in[1]);
        break;

      case 5:
        Serial.print("RF_CH REGISTER RF Channel = ");
        Serial.println(data_in[1]);
        break;

      case 6:
        Serial.print("RF_SETUP REGISTER RF Setup Regsiter = ");
        Serial.println(data_in[1]);
        Serial.print("RF Power = ");
        Serial.print(bitRead(data_in[1], 2));
        Serial.println(bitRead(data_in[1], 1));
        Serial.print("RF_DR_HIGH = ");
        Serial.println(bitRead(data_in[1], 3));
        Serial.print("PLL_LOCK = ");
        Serial.println(bitRead(data_in[1], 4));
        Serial.print("RF_DR_LOW = ");
        Serial.println(bitRead(data_in[1], 5));
        Serial.print("CONT_WAVE = ");
        Serial.println(bitRead(data_in[1], 7));
        break;

      case 7:
        Serial.print("STATUS REGISTER = ");
        Serial.println(data_in[1]);
        Serial.print("TX_FULL = ");
        if (bitRead(data_in[1], 0))
          Serial.println("TX FIFO full");
        else
          Serial.println("TX FIFO not full");

        Serial.print("RX_P_NO = ");
        if (bitRead(data_in[1], 1) && (data_in[1], 2) && (data_in[1], 3))
          Serial.println("RX FIFO empty");
        else
          Serial.println(bitRead(data_in[1], 1) + (bitRead(data_in[1], 2) << 1) + (bitRead(data_in[1], 2) << 2));
        Serial.print("MAX_RT Interrupt = ");
        Serial.println(bitRead(data_in[1], 4));
        Serial.print("TX_DS Interrupt = ");
        Serial.println(bitRead(data_in[1], 5));
        Serial.print("RX_DR Interrupt = ");
        Serial.println(bitRead(data_in[1], 6));
        break;

      case 8:
        Serial.print("OBSERVE_TX REGISTER Transmit observe register = ");
        Serial.println(data_in[1]);
        Serial.print("ARC_CNT = ");
        Serial.println(bitRead(data_in[1], 0) + (bitRead(data_in[1], 1) << 1) + (bitRead(data_in[1], 2) << 2) + (bitRead(data_in[1], 3) << 3));
        Serial.print("PLOS_CNT = ");
        Serial.println(bitRead(data_in[1], 4) + (bitRead(data_in[1], 5) << 1) + (bitRead(data_in[1], 6) << 2) + (bitRead(data_in[1], 7) << 3));
        break;

      case 9:
        Serial.print("RFD REGISTER Received Power Detector = ");
        Serial.println(bitRead(data_in[1], 0));
        break;

      case 10:
        Serial.print("RX_ADDR_P0 LSB = ");
        Serial.println(data_in[1]);
        break;

      case 11:
        Serial.print("RX_ADDR_P1 LSB = ");
        Serial.println(data_in[1]);
        break;

      case 12:
        Serial.print("RX_ADDR_P2 LSB = ");
        Serial.println(data_in[1]);
        break;

      case 13:
        Serial.print("RX_ADDR_P3 LSB = ");
        Serial.println(data_in[1]);
        break;

      case 14:
        Serial.print("RX_ADDR_P4 LSB = ");
        Serial.println(data_in[1]);
        break;

      case 15:
        Serial.print("RX_ADDR_P5 LSB = ");
        Serial.println(data_in[1]);
        break;

      case 16:
        Serial.print("TX_ADDR_LSB = ");
        Serial.println(data_in[1]);
        break;

      case 17:
        Serial.print("RX_PW_P0 RX payload = ");
        Serial.println(data_in[1]);
        break;

      case 18:
        Serial.print("RX_PW_P1 RX payload");
        Serial.println(data_in[1]);
        break;

      case 19:
        Serial.print("RX_PW_P2 RX payload");
        Serial.println(data_in[1]);
        break;

      case 20:
        Serial.print("RX_PW_P3 RX payload");
        Serial.println(data_in[1]);
        break;

      case 21:
        Serial.print("RX_PW_P4 RX payload");
        Serial.println(data_in[1]);
        break;
    
     case 22:
        Serial.print("RX_PW_P5 RX payload");
        Serial.println(data_in[1]);
        break;
    case 23:
        Serial.print("FIFO_STATUS Regisger = ");
        Serial.println(data_in[1]);
        Serial.print("RX_EMPTY = ");
        if (bitRead(data_in[1], 0))
          Serial.println("RX FIFO empty");
        else
          Serial.println("Data in RX FIFO");

        Serial.print("RX EMPTY = ");
        if (bitRead(data_in[1], 4))
            Serial.println("RX FIFO full");
        else
          Serial.println("Available location in RX FIFO");

        Serial.print("TX_FULL = ");
        if (bitRead(data_in[1], 5))
          Serial.println("TX FIFO full");
        else
          Serial.println("Available location in TX FIFO");

        Serial.print("TX_REUSE = ");
        Serial.println(bitRead(data_in[1], 6));
        break;
    }
  }
}
