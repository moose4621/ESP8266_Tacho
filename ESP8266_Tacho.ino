#include <interrupts.h>;

int RPMPin = 4;
int interruptPin = 2;

volatile uint32_t dTimeUS; //measured delta time between interrupts
volatile uint32_t lTimeUS; //last absolute micros
volatile bool tAvail; //signals a new measured delta time is available 
std::vector<uint32_t> tArray;
constexpr size_t aSize = 5;

void setup()
{
  Serial.begin(115200);

  pinMode(RPMPin, INPUT);//_PULLUP);
  pinMode(interruptPin, OUTPUT);
 // analogWriteFreq(3920);//set pwm freq of 3.92Khz for ESP8266
  
  lTimeUS = micros(); //init for the delta calc
  //Digital Pin RPM Set As An Interrupt
  attachInterrupt(RPMPin, fan_interrupt, FALLING);

 
  tArray.reserve(aSize);
}


//Capture The IR Break-Beam Interrupt
ICACHE_RAM_ATTR void fan_interrupt()
{
  uint32_t curr = micros();
  dTimeUS = curr - lTimeUS;
  lTimeUS = curr;
  tAvail = true;
}


bool pushTime() 
{
  
  uint32_t dTimeSafe;
  {
      using namespace esp8266;
      InterruptLock lock;
      dTimeSafe = dTimeUS;
  } 
digitalWrite(interruptPin, !digitalRead(interruptPin));
  if(tArray.size() == aSize) 
  {
      //arr is full, shift elements down, add new time at the end 
      //n Sample Moving Average To Smooth Out The Data
      size_t i = 0;
      for( ; i < tArray.size() - 1; ++i) 
         tArray[i] = tArray[i + 1];


      tArray[i] = dTimeSafe;
   } 
   else
   {
      //arr is not full, just append 
      tArray.push_back(dTimeSafe);
   } 
   return tArray.size() == aSize; //allows waiting until the array is full
} 

uint32_t tArraySum() 
{
    uint32_t sum = 0;
    for(const auto t : tArray) 
        sum += t;
    return sum;
} 

double calcRPM() 
{
    //Last aSize Average RPM Counts Equals....
    uint32_t nom = 60 * 1000000 * tArray.size();
    uint32_t den = tArraySum() * 36;
    return (double) nom / den;
} 

//Main Loop To Calculate RPM
void loop()
{
   // Serial.ptint('l');//debug,means loop
    if (tAvail)
    {
        Serial.print('t');//debug, means time
        tAvail = false;
        if (pushTime()) 
        {
            Serial.print('p');//debug, means push
            double rpm = calcRPM();
          

            Serial.print("RPM = "); Serial.println(rpm);
      
           //Do something with the new rpm val. 
           //Depending on the frequency of the interrupts,
           //you may want to limit this, e.g. do something only 
           //once or twice per second
           //showRPM();
        } 
    Serial.println();
    } 
}
