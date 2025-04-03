#define button 47
#define rand1 A0 //pin for random number generator
#define rand2 A1 

int constEMpin[16] = {22,24,26,28,30,32,34,36,23,25,27,29,31,33,35,37}; // pin for eletromagnets (1 to 16)
int dupEMpin[16] = {22,24,26,28,30,32,34,36,23,25,27,29,31,33,35,37}; // copy of EMpin
int EMpinLength = sizeof(constEMpin) / sizeof(constEMpin[0]);
int buzzerPin[4] = {2,4,6,8}; // buzzerPin[area]

int buzzerNum = 4; // amount of buzzer
int buzztone = 100; // tone the buzzer makes
int buzzDuaration = 100; // how long the buzzer lasts

const int moneyMax = 100; // max amount
const int moneyMin = 5; // min amount
const int stickNum = 16;  // amount of stick

//time unit is milliseconds
const int avgStickDurationMax = 8000; // maximum average stick duration 
const int avgStickDurationMin = 1000; // minimum average stick duration
unsigned long startTime; // starting timer

const int thresholdDeterminer = 3; // variable to determine threshold value
const int rangeSplit = 4; // variable to determine range tolerance

String status = "hibernating, /s to start";
String status_temp = "";
bool setupConfirmed = false;
int money = 0;
int* stickOrderArr; // EMpin order
int* stickDurationArr; // stick duration
bool buzzed = false;
int stickDropped = 0; // counter for dropped sticks

int* moneyToDuration (int money) {
    randomSeed(analogRead(rand1)); // random seed init
    int avgStickDuration = map (money, moneyMin, moneyMax, avgStickDurationMin, avgStickDurationMax); //maping money to average stick duration
    long fullDuration = stickNum * avgStickDuration; // full round duration
    int threshold = avgStickDuration / thresholdDeterminer; // single stick duration's range length / 2
    int stickDurationMax = avgStickDuration + threshold; // single stick duration's maximum time 
    int stickDurationMin = avgStickDuration - threshold; // single stick duration's minimum time
    int rangeTolerance = threshold / rangeSplit * 2; // tolerance in range
    static int stickDurationArr[stickNum]; // array for each stick's duration

    for(int i = 0; i < stickNum; i++) stickDurationArr[i] = stickDurationMin; // filling every index with stickDurationMin
    for(int i = -stickNum * rangeSplit / 2; i < 0; i++) { //giving every index random number but still keeps the sum at fullDuration
      int stick = random(stickNum);
      while(stickDurationArr[stick] > stickDurationMax - rangeTolerance) stick = random(stickNum); //if the single stickDuration is greater than stickDurationMax, then random again
      stickDurationArr[stick] += rangeTolerance; 
    }

    return stickDurationArr;
}

int areaCheck (int pin) {
  for(int i = 0; i < stickNum; i++) {
    if(pin == constEMpin[i]) {
      int area = i / 4 + 1; // area1: 22,24,26,28  area2: 30,32,34,36  area3: 23,25,27,29  area4: 29,31,33,35
      return area;
    }
  }
}

void swap (int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int* shuffle (int arr[]) {
    randomSeed(analogRead(rand2)); //random seed init
    for (int i = EMpinLength-1; i > 0; i--) {
        long j = random(0, EMpinLength);
        swap(&arr[i], &arr[j]);
    }
    return arr;
}

void printArr (String title, int arr[], int length, bool sum) {
  long s = 0;
  Serial.print(title);
  Serial.print(": {"); 
  for(int i = 0; i < length; i++) { // print stick order
    Serial.print(arr[i]);
    Serial.print(",");
    s += arr[i];
  }
  Serial.print("}");
  if(sum) {
    Serial.print(" sum=");
    Serial.print(s);
  }
  Serial.print("\n");
}

void printMultipleValues(int arr[], int valueCount) {
  for(int i = 0; i < valueCount; i++) {
    Serial.print(arr[i]);
    Serial.print(" ");
  }
  Serial.print("\n");
}

String readSerial() {
  String input = ""; 
  while (Serial.available()) { //如果有輸入 則進行紀錄
    input += Serial.readString() ;
  }
  return input;
}

struct buzzer {

  void init () {
    for(int i = 0; i < buzzerNum; i++) {
      pinMode(buzzerPin[i], OUTPUT);
    }
  }

  void On (int area, int note) {
    analogWrite(buzzerPin[area],note);
  }

  void Off (int area) {
    analogWrite(buzzerPin[area],0);
  }

}BZ;

struct electromagnet {
  
  void init() {
    for(int i = 0; i < stickNum; i++) {
      pinMode(constEMpin[i], OUTPUT);
    }
  }

  void on (int pin) {
    digitalWrite(pin, LOW); // because of using a relay, the value for turning on the EM should be LOW  
  }

  void off (int pin) {
    digitalWrite(pin, HIGH); // because of using a relay, the value for turning off the EM should be HIGH 
  }

  void onAll () {
    for(int i = 0; i < stickNum; i++) {
      on(constEMpin[i]);
    }
  }
  
  void offAll () {
    for(int i = 0; i <stickNum; i++) {
      off(constEMpin[i]);
    }
  }

}EM;

void reset() {
  EM.offAll();
  money = 0;
  stickDropped = 0;
  buzzed = false;
  setupConfirmed = false;
}

void play () { //undone
  if(!buzzed) {
    if( (stickDurationArr[stickDropped] / 2 - buzzDuaration) <= millis() - startTime 
          && millis() - startTime <= (stickDurationArr[stickDropped] / 2 - buzzDuaration)) {
      int area = areaCheck(stickOrderArr[stickDropped]);
      BZ.On(area, buzztone);

      Serial.print(area);
      Serial.print(" ");
      buzzed = true;
    } 
  }
  

  if(millis() - startTime >= stickDurationArr[stickDropped]) { // stick drop process
    EM.on(stickOrderArr[stickDropped]);
    if(stickDropped>=1) EM.off(stickOrderArr[stickDropped-1]);

    int valuesArr[3] = {stickDropped, stickOrderArr[stickDropped], millis() - startTime};
    printMultipleValues(valuesArr,3);
    buzzed = false;
    stickDropped++;
    startTime = millis();
  }
}

//--------------------------------main program--------------------------------

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);
  
  BZ.init();
  EM.init();

  EM.offAll();
  /*EM.onAll();
  startTime = millis();
  stickOrderArr = shuffle(dupEMpin); // random EMpin order
  stickDurationArr = moneyToDuration(1); // transfer 50 dollars to stickDuration
  stickDropped = 0; // counter for dropped stick

  Serial.println(stickNum); // print stick amount
  
  printArr("order", stickOrderArr, stickNum, 0); // print stick order
  printArr("duration", stickDurationArr, stickNum, 1); // print stick duration
  */
}

void loop() {
  bool buttonStatus = digitalRead(button); // button pressed?
  if(Serial.available()) {
    String message = readSerial();
    if(message[0] == '/') {
      if(message.length() == 2) {                         
        char command = message[1];
        switch(command) {
          case 'h': // hibernate
            reset();
            EM.offAll();
            status = "hibernating, /s to start";            
            break;

          case 's': // start (basically same as reset)
            reset(); 
            status = "on (reseted), awaiting money input";
            break;

          case 'r': // reset
            reset();
            status = "reseted, awaiting money input";
            break; 

          case 'c': // confirm input waiting for button press
            stickOrderArr = shuffle(dupEMpin); // random EMpin order
            stickDurationArr = moneyToDuration(money); // transfer dollars to stickDuration
            stickDropped = 0; // counter for dropped stick
            printArr("order", stickOrderArr, stickNum, 0); // print stick order
            printArr("duration", stickDurationArr, stickNum, 1); // print stick duration
            setupConfirmed = true;
            status = "setup confirmed, awaiting play request";
            break;
          
          case 'p':
            if(setupConfirmed) {
              startTime = millis();
              status = "Game started"; 
            }else status = "waiting for confirmation";
            break;

          default: // invalid command
            Serial.println("Invalid Command");
            break;

        }
      }else Serial.println("Invalid Command"); // error
    }else{
      bool numerical = true;
      for(int i = 0; i < message.length(); i++) {
        if(!('0' <= message[i] && message[i] <= '9')) numerical = false;
      }
      if(numerical) {
        money = message.toInt();
        if(moneyMin <= money && money <= moneyMax) {
          Serial.print("Money: ");
          Serial.println(money);
          status = "money set up, /c to confirm";
        }else{
          money = 0; 
          Serial.println("Invalid Money");
        } 
      }else Serial.println("Invalid Input");
    }
  }
  if(status == "Game started") {
    play();
    if(stickDropped == 16) {
      reset();
      status = "Game ended, auto reseted, awaiting money input";
    }
  }
  if(status != status_temp) {
    Serial.print("status: ");
    Serial.println(status);
    status_temp = status;
  }
  //status update


  /*if(millis() - startTime >= stickDuration[stickDropped]) {
    EM.off(stickOrder[stickDropped]);
    Serial.print(stickDropped);
    Serial.print(" ");
    Serial.print(stickOrder[stickDropped]);
    Serial.print(" ");
    Serial.print(stickDuration[stickDropped]);
    Serial.print(" ");
    Serial.print(areaCheck(stickOrder[stickDropped]));
    Serial.print(" ");
    Serial.println(startTime);
    stickDropped++;
    startTime = millis();
    delay(100);
  }*/

}