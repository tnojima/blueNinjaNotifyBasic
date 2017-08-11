/* //<>// //<>//
 * Arduino - Processingシリアル通信
 * センサーの値をグラフにプロット
 * Processing側サンプル
 http://yoppa.org/tau_bmaw13/4790.html
 Multiple window sample
 http://3846masa.blog.jp/archives/1038375725.html
 */
import processing.serial.*;
import hypermedia.net.*;
final int PORT = 8080;
int FRAME_RATE=15;

UDP udp;
String log;

///////////////////////
/// Magnet Data Related 
///////////////////////
int[] adcData=new int[4];
ADCDataWindow adc;


///////////////////////
/// Magnet Data Related 
///////////////////////
float[] accData=new float[3];
AccDataWindow acc;

///////////////////////
/// Magnet Data Related 
///////////////////////
float[] gyroData=new float[3];
GyroDataWindow gyro;

///////////////////////
/// Magnet Data Related 
///////////////////////
int[] mgData=new int[3];
MagDataWindow mag;

///////////////////////
/// Air Data Related 
///////////////////////
float[] airData=new float[2];
AirDataWindow air;

// グラフの線の色を格納
color[] col = {
  color(255, 127, 31), 
  color(31, 255, 127), 
  color(127, 31, 255), 
  color(31, 127, 255), 
  color(127, 255, 31), 
  color(127, 127, 0), 
  color(127, 127, 0), 
  color(255, 0, 0), 
  color(0, 255, 0), 
  color(0, 0, 255), 
  color(127, 0, 0), 
  color(0, 127, 0)
};

void setup() {

  udp = new UDP(this, PORT);
  udp.listen(true);


  air = new AirDataWindow(this);
  mag = new MagDataWindow(this);
  gyro = new GyroDataWindow (this);
  acc = new AccDataWindow (this);
  adc = new ADCDataWindow(this);
}

void draw() {
}


void receive(byte data[]) {
  int i=0;
  int prevSeparator=0;  /// Position of separator
  int j;
  int dataPoint=0;
  String  flag="";
  String  byteBuf="";
  byte separator=',';
  boolean first_separator_has_come=false;
  boolean exitFlag=false;
  while (!exitFlag) {
    if (data[i] == '\n') {
      data[i] =',';
      exitFlag=true;
    }
    if (!first_separator_has_come) {  
      if (data[i]==separator) {             // first separator has come. flag section has completed
        for (j=0; j<4; j++) {                       // flag is composed of 4 characters
          flag +=str(char(data[j]));
        }
        flag = flag.replaceAll(",", " "); // replace "," to single space
        flag = trim(flag);                          // remove spaces and extra control characters
        first_separator_has_come=true;
        prevSeparator=i;
        dataPoint=0;
      }
    } else {
      /// parse data value
      if (data[i]==separator) {   //separator has come. A set of data has completed
        for (j=prevSeparator+1; j<i-1; j++) {
          byteBuf+=str(char(data[j]));
        }
        byteBuf+=str(char(data[i-1]));


        if (flag.equals("MTN")) {
          if (dataPoint<3) {
            accData[dataPoint]=float(int(byteBuf))/2048;
          } else if (dataPoint<6) {
            gyroData[dataPoint-3]=float(int(byteBuf))/16.4;
          } else {
            mgData[dataPoint-6]=int(byteBuf);
          }
        } else if (flag.equals("AIR")) {
          if (dataPoint==0) {
            //dataPoint is always 0 
            airData[dataPoint]=0.01*int(byteBuf);
          } else {
            // convert to hect pascal
            airData[dataPoint]=float(int(byteBuf))/256/100;
          }
        } else if (flag.equals("ADC")) {
          adcData[dataPoint]=int(byteBuf);
        }
        byteBuf="";
        prevSeparator=i;
        dataPoint++;
      }
    }
    i++;
  }

  byteBuf="";
  prevSeparator=0;
  flag="";
}