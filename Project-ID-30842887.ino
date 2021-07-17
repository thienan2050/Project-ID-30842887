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
unsigned int timerCounterDown, lastTimerCounterDown = 432;  //Only 16bit, not 32 as in Linux
STATE StateMachine, lastStateMachine;
VALVE_STATE valveState = INIT;
int pause, turnOffValve, currentState[7];
bool flagValve = false, flagNC_SWITCH = false;
LiquidCrystal LCD(8, 9, 4, 5, 6, 7);        // Select the pins used on the LCD panel

/* Timer interrupt handler. */
ISR(TIMER1_COMPA_vect){
  TCNT1  = 0;               //First, set the timer back to 0 so it resets for next interrupt
  if(timerCounterDown != 0)
  {
    timerCounterDown--;
    //Serial.print("Time remaining : ");
    //Serial.println(timerCounterDown);
  }
  if(turnOffValve != 0)
    turnOffValve--; 
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

    
    StateMachine = ON;
    lastStateMachine = PRODUCE; // Dont care, just a garbage value. */
    timerCounterDown = SMALL;
    
    LCD.begin(16, 2);       // Start the library
    LCD.setCursor(0, 0);    // Set the LCD cursor position
}

void loop()
{
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
        }
        
        else if((digitalRead(BUTTON2) == HIGH)&&(valveState == FULL))
        {
          valveState = INIT;
          StateMachine = ON;
          timerCounterDown = DELAY;
          digitalWrite(VALVE, LOW);
          Serial.println("ICE WAS REMOVED WITHIN 5S, START NEW LOOP ");
        }
        else if((digitalRead(BUTTON2) == HIGH)&&(valveState == ON_ALARM))
        {
          valveState = INIT;
          StateMachine = ON;
          timerCounterDown = DELAY;
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
      }
      break;
      case OFF:
      {
        if ((digitalRead(COMPRESSOR) == HIGH)&&(digitalRead(PUMP) == HIGH))
        {
          digitalWrite(COMPRESSOR, LOW);
          digitalWrite(PUMP, LOW);
        }
      }
      break;
      case PRODUCE:
      {
        if((digitalRead(VALVE) == LOW)&&(valveState == INIT))
        {
          Serial.println("TURN ON VALVE, WAIT BUTTON2 GOES HIGH");
          digitalWrite(VALVE, HIGH);
        }
        //if((turnOffValve == 0)&&(valveState == FULL))
        //  digitalWrite(VALVE, LOW);
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
