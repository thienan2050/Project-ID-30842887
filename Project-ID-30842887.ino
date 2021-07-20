#include <LiquidCrystal.h>              // LCD library 

/* Calculate in ms. */
//#define SMALL  5000
//#define MEDIUM 7000
//#define LARGE  8000
//#define DELAY  5000

#define SMALL  10
#define MEDIUM 14
#define LARGE  16
#define DELAY  10


/* Compressor */
#define COMPRESSOR A5
/* Water pump */
#define PUMP 2
/* Heat Valve */
#define VALVE 3
/* Alarm */
#define ALARM 11
/* Button2 */
#define BUTTON2 12
#define NC_SWITCH 13


/*
 *  ON state : turn on compressor and water pump, delay in xxx
 * OFF state : turn off compressor and water pump, delay in xxx
 * PRODUCE state : ice fall and touch button2, delay 5000.
*/
typedef enum { ON, OFF, PRODUCE, LOW_WATER } STATE;
typedef enum { INIT, WAIT_FALLING, FULL, ON_ALARM, COMPLETE } VALVE_STATE;
typedef enum { RIGHT, UP, DOWN, LEFT, SELECT, NOISE } BUTTON;
int TIMER[3] = {SMALL, MEDIUM, LARGE};
unsigned int timerCounterDown, lastTimerCounterDown = 432;  //Only 16bit, not 32 as in Linux
STATE StateMachine, lastStateMachine;
VALVE_STATE valveState = INIT;
int pause, turnOffValve, lastTurnOffValve, currentState[7], wait;
bool flagValve = false, flagNC_SWITCH = false;
int index = 0, lastIndex = 4;
bool configuration = false;
bool done = false;
LiquidCrystal LCD(8, 9, 4, 5, 6, 7);        // Select the pins used on the LCD panel

/* Timer interrupt handler. */
ISR(TIMER1_COMPA_vect){
  TCNT1  = 0;               //First, set the timer back to 0 so it resets for next interrupt
  if(timerCounterDown != 0)
  {
    timerCounterDown--;
  }
  if(turnOffValve != 0)
    turnOffValve--; 
  if(wait != 0)
  {
    wait--;
  }
}

BUTTON readKeyPad(void)
{
  int value = analogRead(A0);
  if(value < 60)
  { 
    delay(100);
    if(analogRead(A0) > 900)
    {
      Serial.println("RIGHT");
      return RIGHT;
    }
  }
  else if(value < 200)
  {
    delay(100);
    if(analogRead(A0) > 900)
    {
      //Serial.println("UP");
      return UP;
    }
  }
  else if (value < 400)
  {
    delay(100);
    if(analogRead(A0) > 900)
    {
      //Serial.println("DOWN");
      return DOWN;
    }
  }
  else if (value < 600)
  {
    delay(100);
    if(analogRead(A0) > 900)
    {
      Serial.println("LEFT");
      return LEFT;
    }
  }
  else if (value < 800)
  {
    delay(100);
    if(analogRead(A0) > 900)
    {
      Serial.println("SELECT");
      return SELECT;
    }
  }
  return NOISE;
}

char *getSizeByIndex(int index)
{
  if(index == 0)
    return "SIZE SMALL";
  else if(index == 1)
  {
    return "SIZE MEDIUM";
  }
  else if(index == 2)
  {
    return "SIZE LARGE";
  }
  return "";
}

char *getState(STATE state)
{
  if(state == 0)
    return "ON...";   //Sorry, it's too long so you should choose what to be displayed here by yourself. 
  else if(state == 1)
    return "OFF...";
  else if(state == 2)
    return "PRODUCE ICE";
  return "";
}

void setup()
{
    /* Setup timer interrupt.
  / For more details, visit https://electronoobs.com/eng_arduino_tut140.php */
    cli();                  // Stop all interrupt so we can set up.
    TCCR1A = 0;             // Reset entire TCCR1A to 0.
    TCCR1B = 0;             // Reset entire TCCR1B to 0.
    TCCR1B |= B00000100;    // Set prescalar to 256.
    TIMSK1 |= B00000010;    // Set OCIE1A to 1 to enable compare match A.
    OCR1A = 6250;           // Equivalent to 100ms.
    sei();                  // Enable the interrupts.
    /* End of config timer interrupt. */
  
    /* Setup serial for printing log. */
    Serial.begin(9600);


    /* Configuration for others. */
    pinMode(COMPRESSOR, OUTPUT);
    pinMode(PUMP, OUTPUT);
    pinMode(VALVE, OUTPUT);
    pinMode(ALARM, OUTPUT);

    /* Configuration for button2 */
    pinMode(BUTTON2, INPUT);
    pinMode(NC_SWITCH, INPUT);

    LCD.begin(16, 2);             // LCD 16x2
}
void loop()
{
    if((readKeyPad() == SELECT)&&(configuration == true))
    {
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print("RECONFIG SIZE ?");
      LCD.setCursor(0, 1);
      LCD.print("SELECT TO OK");
      wait = 10;         // WAIT 10 TO RECORD A SECOND PRESS, IF TIME'S UP, RESUME
      while((readKeyPad() != SELECT)&&(wait != 0));
      if(wait == 0)
      {
        Serial.println("RESUME");
        LCD.clear();
        LCD.print("RESUME");
        lastStateMachine = 10;
      }
      else if(wait > 0)
      {
        LCD.clear();
        LCD.print("RECONFIGURE");
        configuration = false;
        digitalWrite(COMPRESSOR, LOW);
        digitalWrite(VALVE, LOW);
        digitalWrite(PUMP, LOW);
        digitalWrite(VALVE, LOW);
        delay(20);
      }
    }
    while(configuration == false)
    {
      LCD.clear();
      LCD.setCursor(4, 0);          // Colum 0, Row 0
      Serial.println("WELCOME");    
      LCD.print("WELCOME");
      LCD.setCursor(0, 1);          // Colum 0, Row 1
      Serial.println("PRESS SELECT BUTTON TO CONTINUE");
      LCD.print("PRESS SELECT TO CONT");
      while(readKeyPad() != SELECT);
      LCD.clear();
      LCD.setCursor(3, 0);
      LCD.print(getSizeByIndex(index));
      delay(500);
      BUTTON temp;
      done = false;
      //while(readKeyPad() != SELECT)
      while(!done)
      {
        temp = readKeyPad();
        if(temp == DOWN)
        {
          index = (index + 1) % 3;
          //Serial.println("DOWN");
        }
        else if(temp == UP)
        {
          index = (index - 1) % 3;
          //Serial.println("UP");
        }
        if(index == -1) index = 2;
        if(index == -2) index = 1;
        if(index != lastIndex)
        {
          LCD.setCursor(7, 0);
          LCD.print("       ");
          LCD.setCursor(3, 0);
          LCD.print(getSizeByIndex(index));
          Serial.println(getSizeByIndex(index));
        }
        lastIndex = index;
        if(temp == SELECT)
          done = true;
      }
      Serial.println("Choose size ");
      Serial.println(TIMER[index]);
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print(getSizeByIndex(index));
      LCD.setCursor(0, 1);
      LCD.print("TIME = ");
      LCD.setCursor(7, 1);
      LCD.print(TIMER[index]);
      timerCounterDown = TIMER[index];
      StateMachine = ON;
      lastStateMachine = PRODUCE; // Dont care, just a garbage value. */
      configuration = true;
      valveState = INIT;
    }
    if((digitalRead(NC_SWITCH) == HIGH)&&(flagNC_SWITCH == false))
      {
        flagNC_SWITCH = true;
        currentState[0] = digitalRead(COMPRESSOR);    // Save state of compressor
        currentState[1] = digitalRead(PUMP);          // Save state of pump
        currentState[2] = digitalRead(VALVE);         // Save state of valve
        currentState[3] = timerCounterDown;           // Save state of current timer
        currentState[4] = StateMachine;           
        currentState[5] = turnOffValve ? turnOffValve : 0;      
        currentState[6] = digitalRead(ALARM);    
        LCD.clear();
        LCD.setCursor(0, 0);
        LCD.print("LOW WATER, PLEASE REFILL");           // Show on LCD
        Serial.println("LOW WATER,  PLEASE REFILL");
        digitalWrite(ALARM, HIGH);                       // Turn on alarm.
        StateMachine = LOW_WATER;
        digitalWrite(COMPRESSOR, LOW);
        digitalWrite(PUMP, LOW);
        digitalWrite(VALVE, LOW);
      }   
     if((flagNC_SWITCH == true)&&(digitalRead(NC_SWITCH) == LOW))
     {   
      flagNC_SWITCH = false;
      digitalWrite(COMPRESSOR,currentState[0]);       // Restore saved state of compressor
      digitalWrite(PUMP,currentState[1]);             // Restore saved state of pump
      digitalWrite(VALVE,currentState[2]);            // Restore saved state of valve
      digitalWrite(ALARM,currentState[6]);
      timerCounterDown = currentState[3];             // Restore saved timer
      StateMachine = currentState[4];                 // Restore saved state machine
      turnOffValve = currentState[5];
      lastStateMachine = LOW_WATER;
      LCD.clear();                                    // Clear LCD.
      Serial.println("RESTORE THE CURRENT STATE");
     }


    /* State machine operationg. */
    switch(StateMachine)
    {
      case ON:
       {
        if(timerCounterDown == 0)
        {
          StateMachine = OFF;
          timerCounterDown = DELAY;
        }
       }
      break;
      case OFF:
      {
        if(timerCounterDown == 0)
        {
          StateMachine = PRODUCE;
        }
      }
      break;
      case PRODUCE:
      {
        /* Wait ice fail. */
        if((digitalRead(BUTTON2) == HIGH)&&(valveState == INIT))
        {
          valveState = WAIT_FALLING;
          Serial.println("WAIT ICE FALL");
        }

        /* After 5s, ice isnt removed, so turn alarm. */  
        else if((digitalRead(BUTTON2) == LOW)&&(valveState == FULL)&&(turnOffValve == 0))
        {
            // still turn on valve
            valveState = ON_ALARM;
            
            //LCD.print("ICE FULL, EMPTY !"); //Show on LCD
            Serial.println("TIME'S UP, TURN ON ALARM, TURN OFF COMPRESSOR, PUMP AND VALVE"); 
            LCD.clear();
            LCD.setCursor(0, 0);
            LCD.print("ICE FULL, REMOVE!");
            LCD.setCursor(0, 1);
            LCD.print("TURN OFF ALL");
            digitalWrite(VALVE, LOW);
            digitalWrite(COMPRESSOR, LOW);
            digitalWrite(PUMP, LOW);
            digitalWrite(ALARM, HIGH);
        }
        
        /* 5s after falling */
        else if((digitalRead(BUTTON2) == LOW)&&(valveState == WAIT_FALLING))
        {
          Serial.println("YEAH! ICE HIT THE BUTTON2, WAIT 5S TO BE REMOVED");
          valveState = FULL;
          turnOffValve = DELAY;
          lastTurnOffValve = 3;
        }
        
        else if((digitalRead(BUTTON2) == HIGH)&&(valveState == FULL))
        {
          valveState = INIT;
          StateMachine = ON;
          timerCounterDown = TIMER[index];
          digitalWrite(VALVE, LOW);
          Serial.println("ICE WAS REMOVED WITHIN 5S, START NEW LOOP ");
        }
        else if((digitalRead(BUTTON2) == HIGH)&&(valveState == ON_ALARM))
        {
          valveState = INIT;
          StateMachine = ON;
          timerCounterDown = TIMER[index];
          Serial.println("ICE WAS REMOVED, START NEW LOOP ");
          digitalWrite(ALARM, LOW);
        }
      }
      break;
      default: break;
    }
    /* Action for state machine. */
    switch(StateMachine)
    {
      case ON:
      {
        if ((digitalRead(COMPRESSOR) == LOW)&&(digitalRead(PUMP) == LOW))
        {
          digitalWrite(COMPRESSOR, HIGH);
          digitalWrite(PUMP, HIGH);
        }
        if(StateMachine != lastStateMachine)
        {
          LCD.clear();
          LCD.setCursor(0, 0);
          LCD.print(getState(ON));
        }
        if(timerCounterDown != lastTimerCounterDown)
        {
          LCD.setCursor(8, 1);
          LCD.print("        ");
          LCD.setCursor(0, 1);
          LCD.print("DONE IN ");
          LCD.setCursor(8, 1);
          LCD.print(timerCounterDown);
        }
        
      }
      break;
      case OFF:
      {
        if ((digitalRead(COMPRESSOR) == HIGH)&&(digitalRead(PUMP) == HIGH))
        {
          digitalWrite(COMPRESSOR, LOW);
          digitalWrite(PUMP, LOW);
        }
        if(StateMachine != lastStateMachine)
        {
          LCD.clear();
          LCD.setCursor(0, 0);
          LCD.print(getState(OFF));
        }
        if(timerCounterDown != lastTimerCounterDown)
        {
          LCD.setCursor(8, 1);
          LCD.print("        ");
          LCD.setCursor(0, 1);
          LCD.print("DONE IN ");
          LCD.setCursor(8, 1);
          LCD.print(timerCounterDown);
        }
      }
      break;
      case PRODUCE:
      {
        if((digitalRead(VALVE) == LOW)&&(valveState == INIT))
        {
          Serial.println("TURN ON VALVE, WAIT BUTTON2 GOES HIGH");
          LCD.clear();
          LCD.setCursor(0, 0);
          LCD.print("TURN ON VALVE");
          digitalWrite(VALVE, HIGH);
        }
        if((lastStateMachine == LOW_WATER)&&(flagValve == true))
        {
            Serial.println("ICE FULL");
            LCD.clear();
            LCD.setCursor(0, 0);
            LCD.print("ICE FULL, REMOVE ICE");
        }
        if((digitalRead(BUTTON2) == LOW)&&(valveState == FULL)&&(turnOffValve != 0))
        {
          LCD.setCursor(0, 0);
          LCD.print("ICE FULL, REMOVE!");
          LCD.setCursor(0, 1);
          LCD.print("WAIT : ");
          if(lastTurnOffValve != turnOffValve)
          {
            LCD.setCursor(7, 1);
            LCD.print("         ");
            LCD.setCursor(7, 1);
            LCD.print(turnOffValve);
          }
          lastTurnOffValve = turnOffValve;
        }
      }
      break;
      default:break;
    }
    lastTimerCounterDown = timerCounterDown;





    


    /* When code runs exactly what you want, you can comment this, it's just used to debug. */
    if(StateMachine != lastStateMachine)
    {
      switch(StateMachine)
      {
        case ON:
          Serial.println("TURN ON  COMPRESSOR & PUMP");
        break;
        case OFF:
          Serial.println("TURN OFF COMPRESSOR & PUMP");
        break;
        case PRODUCE:
          {
            if((lastStateMachine == LOW_WATER)&&(flagValve == true))
              Serial.println("ICE FULL");
          }
        break;
        default:break;
      }
    }
    lastStateMachine = StateMachine;
}
