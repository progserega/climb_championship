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

// Отладка (логи выводятся в консоль):
//#define DEBUG 1
#define DEBUG_SLEEP 500

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
  FALSH_START=5
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
  sprintf(buf,"trace:%d; result:%s; time_ms:%lu", trace.id, trace.status, trace.time);
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
      sprintf(debug_buf,"trace:%d; result:DEBUG_STATE_NOT_WORK; time_ms:0",trace.id);
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
      sprintf(debug_buf,"trace:%d; result:DEBUG_STATE_ON_START; time_ms:0",trace.id);
      Serial.println(debug_buf); 
#endif
      buttonState = getStatusKey(start_key);
      if (buttonState == OFF)
      {
        trace.startTime=millis();
        trace.state = PROCESS;
        // turn LED on:    
        digitalWrite(ledPin, HIGH);
      }
      break;
    }
    case PROCESS:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d; result:DEBUG_STATE_PROCESS; time_ms:0",trace.id);
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
    case STOP:
    {
#ifdef DEBUG
      sprintf(debug_buf,"trace:%d; result:DEBUG_STATE_STOP; time_ms:0",trace.id);
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
        if(serialReadBuf[serialReadBufIndex]=='\n')
        {
          // конец переданной строки
          serialReadBuf[serialReadBufIndex]=0;
          serialReadBufIndex=0;
#ifdef DEBUG
        char debug_buf[1024]="";
        sprintf(debug_buf,"trace:%d;result:DEBUG success read serial data - '%s';time_ms:0",serialReadBuf);  
        Serial.println(debug_buf); 
#endif

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
        char debug_buf[1024]="";
        serialReadBuf[serialReadBufIndex-1]=0;
        sprintf(debug_buf,"trace:%d;result:DEBUG serialReadBuf is too small! readed data was - '%s';time_ms:0",serialReadBuf);  
        Serial.println(debug_buf); 
#endif

      }
    }
  }
  return false;
}

int execCommand(char *buf)
{
}

void loop(){

#ifdef DEBUG
  char debug_buf[256]="";
  int button1State = getStatusKey(TRACE_1_START_BUTTON);
  int button2State = getStatusKey(TRACE_1_STOP_BUTTON);
  sprintf(debug_buf,"trace:%d;result:DEBUG key1=%d, key2=%d;time_ms:0",trace1.id, trace1.state,button1State, button2State);  
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
