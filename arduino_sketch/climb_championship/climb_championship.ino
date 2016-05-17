// Отладка (логи выводятся в консоль):
//#define DEBUG 1
#define DEBUG_SLEEP 500

// Уровень сигнала срабатывания кнопки (0-1023):
#define STAGE_LEVEL 950

// Аналоговые входы, к которым подключаются кнопки:
#define TRACE_1_START_BUTTON 0
#define TRACE_1_STOP_BUTTON 1
#define TRACE_2_START_BUTTON 2
#define TRACE_2_STOP_BUTTON 3

// Судейские кнопки начала соревнования:
#define TRACE_1_BEGIN_BUTTON 4
#define TRACE_2_BEGIN_BUTTON 4

// цифровые выходы светодиодов, отображающего свечением, что участник работает на трассе
#define TRACE_1_LED 8
#define TRACE_2_LED 9


// Идентификаторы трасс (отсылаются в программу для идентификации трассы (числовое значение):
#define TRACE_1_ID 0
#define TRACE_2_ID 1


// служебные константы:
#define ON 1
#define OFF 0
#define SUCCESS 1
#define NOT_SUCCESS 0

enum states {
  NOT_WORK=0,
  ON_START=1,
  PROCESS=2,
  STOP=3,
  PREPARE_TO_START=4,
  FALSH_START=5,
  START=6,
  NOT_ON_START=7
} st;

char serialReadBuf[255]="";
int serialReadBufIndex=0;

struct traceStruct {
  int id; // ID трассы
  int state; // текущее состояние STATE-машины трассы (см. enum states)
  int prepare_to_start_key; // номер аналогового входа кнопки старта судьи (только после её нажатия спортсмен может начинать трассу,
// иначе - "фальшстарт"

  int start_key; // номер аналогового входа кнопки старта трассы
  int stop_key; // номер аналогового входа кнопки остановки трассы
  int ledPin; // номер цифрового выхода светодиода, отображающего свечением, что участник работает на трассе
  unsigned long startTime; // время в миллисекундах старта прохождения трассы
  unsigned long stopTime; // время в миллисекундах завершения прохождения трассы
  unsigned long time; // результирующее время прохождения трассы (отправляется в программу на ПК)
  char status[100]; // статус трассы ("falsh_start", "success")
};

traceStruct trace1;
traceStruct trace2;

int getStatusKey(int numKey)
{    
   int sensorValue=0;
   int statusKey=0;
   sensorValue = analogRead(numKey);
   if (sensorValue >= STAGE_LEVEL )
   {
     return ON;
   }
   else
   {
     return OFF;
   }
}

int print_result(struct traceStruct &trace)
{
  char buf[255];
  sprintf(buf,"trace:%d;result:%s;time_ms:%lu;current_log_ms:%lu", trace.id, trace.status, trace.time,millis());
  Serial.println(buf); 
}

int checkStateMachine(struct traceStruct &trace)
{
#ifdef DEBUG 
  char debug_buf[255]="";
#endif
  int start_key=trace.start_key;
  int stop_key=trace.stop_key;
  int prepare_to_start=trace.prepare_to_start_key;
  int ledPin=trace.ledPin;
  int buttonState = 0;
  
  switch (trace.state)
  {
    case NOT_WORK:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d;result:DEBUG_STATE_NOT_WORK;time_ms:0;current_log_ms:%lu",trace.id,millis());
      Serial.println(debug_buf); 
#endif
       break;
    }
    case PREPARE_TO_START:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d;result:DEBUG_STATE_PREPARE_TO_START;time_ms:0;current_log_ms:%lu",trace.id,millis());
      Serial.println(debug_buf); 
#endif
      buttonState = getStatusKey(start_key);
      if (buttonState == ON)
      {
        trace.state = ON_START;
      }
      break;
    }
    case ON_START:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d;result:DEBUG_STATE_ON_START;time_ms:0;current_log_ms:%lu",trace.id,millis());
      Serial.println(debug_buf); 
#endif
      buttonState = getStatusKey(start_key);
      if (buttonState == OFF)
      {
        trace.state = FALSH_START;
        trace.stopTime=0;
        trace.startTime=0;
        sprintf(trace.status,"falsh_start");
        return SUCCESS;
      }
      break;
    }
    case FALSH_START:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d;result:DEBUG_FALSH_START;time_ms:0;current_log_ms:%lu",trace.id,millis());
      Serial.println(debug_buf); 
#endif
      //trace.state = NOT_WORK;
      // result:
      break;
    }
    case START:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d;result:DEBUG_STATE_START;time_ms:0;current_log_ms:%lu",trace.id,millis());
      Serial.println(debug_buf); 
#endif
  /*    buttonState = getStatusKey(start_key);
      if (buttonState == OFF)
      {*/
        trace.startTime=millis();
        trace.state = PROCESS;
        // turn LED on:    
        digitalWrite(ledPin, HIGH);
      //}
      break;
    }
    case PROCESS:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d;result:DEBUG_STATE_PROCESS;time_ms:0;current_log_ms:%lu",trace.id,millis());
      Serial.println(debug_buf); 
#endif
      buttonState = getStatusKey(stop_key);
      if (buttonState == ON)
      {
        trace.stopTime=millis();
        trace.time=trace.stopTime-trace.startTime;
        trace.state = STOP;
        // turn LED on:    
        digitalWrite(ledPin, LOW);
      }
      break;
    }
    case NOT_ON_START:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d;result:DEBUG_NOT_ON_START;time_ms:0;current_log_ms:%lu",trace.id,millis());
      Serial.println(debug_buf); 
#endif
      trace.state=NOT_WORK;
      trace.time=0;
      sprintf(trace.status,"not_on_start_button");
      return SUCCESS;
    }
    case STOP:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d;result:DEBUG_STATE_STOP;time_ms:0;current_log_ms:%lu",trace.id,millis());
      Serial.println(debug_buf); 
#endif
      buttonState = getStatusKey(stop_key);
      if (buttonState == OFF)
      {
        trace.state = NOT_WORK;
        // result:
        trace.stopTime=0;
        trace.startTime=0;
        sprintf(trace.status,"success");
        return SUCCESS;
      }
      break;
    }
  }
  return NOT_SUCCESS;
}

void setup() {
  // устанавливаем параметры последовательного порта:
  Serial.begin(9600);
  
  // инициализируем структуры трасс:
  memset(&trace1,0,sizeof(trace1));
  memset(&trace2,0,sizeof(trace1));
  
  trace1.state=NOT_WORK;
  trace2.state=NOT_WORK;
  
  trace1.start_key=TRACE_1_START_BUTTON;
  trace1.stop_key=TRACE_1_STOP_BUTTON;
  trace2.start_key=TRACE_2_START_BUTTON;
  trace2.stop_key=TRACE_2_STOP_BUTTON;
  
  trace1.prepare_to_start_key=TRACE_1_BEGIN_BUTTON;
  trace2.prepare_to_start_key=TRACE_2_BEGIN_BUTTON;
  
  trace1.ledPin=TRACE_1_LED;
  trace2.ledPin=TRACE_2_LED;
  
  trace1.id=TRACE_1_ID;
  trace2.id=TRACE_2_ID;
  
  // по умолчанию - ошибка:
  sprintf(trace1.status,"error");
  sprintf(trace2.status,"error");
  
  // initialize the LED pin as an output:
  pinMode(trace1.ledPin, OUTPUT);      
  pinMode(trace2.ledPin, OUTPUT);      
}

int serialRead(void)
{
  int num = Serial.available();
  if(num > 0)
  {
    for(int index=0;index<num;index++)
    {
      // проверка на переполнение буфера
      if (serialReadBufIndex < sizeof(serialReadBuf) )
      {
        serialReadBuf[serialReadBufIndex]=Serial.read();
        //Serial.write(serialReadBuf[serialReadBufIndex]);
        if(serialReadBuf[serialReadBufIndex]=='\n')
        {
          // конец переданной строки
          serialReadBuf[serialReadBufIndex]=0;

#ifdef DEBUG
        char debug_buf[255]="";
        sprintf(debug_buf,"trace:0;result:DEBUG success read serial data - '%s', serialReadBufIndex='%d';time_ms:0;current_log_ms:%lu",serialReadBuf,serialReadBufIndex,millis());  
        Serial.println(debug_buf); 
#endif
          serialReadBufIndex=0;
          return true;
        }
        else
        {
          serialReadBufIndex++;
        }
      }
      else
      {
        // переполнение буфера
#ifdef DEBUG
        char debug_buf[255]="";
        serialReadBuf[serialReadBufIndex-1]=0;
        sprintf(debug_buf,"trace:0;result:DEBUG serialReadBuf is too small! readed data was - '%s';time_ms:0;current_log_ms:%lu",serialReadBuf,millis());  
        Serial.println(debug_buf); 
#endif

      }
    }
  }
  return false;
}

int execCommand(char *buf)
{
  if(!strcasecmp(buf,"vnimanie"))
  {
    // внимание
    trace1.state=PREPARE_TO_START;
    trace2.state=PREPARE_TO_START;
  }
  else if (!strcasecmp(buf,"start"))
  {
    // start
    if (trace1.state==ON_START)
    {
      trace1.state=START;
    }
    else if (trace1.state!=FALSH_START)
    {
      // участник не на стартовой кнопке:
      trace1.state=NOT_ON_START;
    }
    if (trace2.state==ON_START)
    {
      trace2.state=START;
    }
    else if (trace2.state!=FALSH_START)
    {
      // участник не на стартовой кнопке:
      trace2.state=NOT_ON_START;
    }
  }
  else
  {
#ifdef DEBUG
    char debug_buf[256]="";
    sprintf(debug_buf,"trace:0;result:DEBUG unknown command - '%s';time_ms:0;current_log_ms:%lu",buf,millis());  
    Serial.println(debug_buf); 
#endif
    return false;
  }
  return true;
}

void loop(){

#ifdef DEBUG
  char debug_buf[256]="";
  int button1State = getStatusKey(TRACE_1_START_BUTTON);
  int button2State = getStatusKey(TRACE_1_STOP_BUTTON);
  sprintf(debug_buf,"trace:%d;result:DEBUG key1=%d, key2=%d;time_ms:0;current_log_ms:%lu",trace1.id, button1State, button2State,millis());  
  Serial.println(debug_buf); 
  delay(DEBUG_SLEEP);
#endif
  if(checkStateMachine(trace1) == SUCCESS)
  {
    print_result(trace1);
  }
  if(checkStateMachine(trace2) == SUCCESS)
  {
    print_result(trace2);
  }
  if(serialRead())
  {
    execCommand(serialReadBuf);
  }
}
