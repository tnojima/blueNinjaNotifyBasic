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

int NUM = 11; //センサーの数
UDP udp;
String log;

float[] accData=new float[3];
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

int[] sensors_min= {-16, -16, -16, -2000, -2000, -2000, -500, -500, -500, 0, 0};
int[] sensors_max={  16, 16, 16, 2000, 2000, 2000, 500, 500, 500, 10, 2};
float[] prevTx=new float[NUM];
float[] prevTy=new float[NUM];
String[] label={"GX", "GY", "GZ", "GRX", "GRY", "GRZ", "MGX", "MGY", "MGZ", "TEMP", "AIRP"};

int cnt; //カウンター

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
  //画面設定
  size(1024, 768);
  frameRate(60);

  udp = new UDP(this, PORT);
  udp.listen(true);

  //グラフ初期化
  initGraph();
  
   air = new AirDataWindow(this);
   mag = new MagDataWindow(this);
   gyro = new GyroDataWindow (this);

}

void draw() {
  // センサーの数だけ、グラフをプロット  
  float data=0;
  for (int i = 0; i < NUM; i++) {
    if (i<3) {
      data=accData[i];
    }else if (i<6) {
      data=gyroData[i-3];
    } else if (i<9) {
      data=float(mgData[i-6]);
    } else if (i<11) {
      data=airData[i-9];
    }

    float tx = map(cnt, 0, width, 0, width);
    float ty = map(data, sensors_min[i], sensors_max[i], height/NUM, 0)+height/NUM*i;
    fill(0);
    text(label[i], 0, height/NUM*(i+0.5));
    text(data, 30, height/NUM*(i+0.5));

    //ellipse(tx, ty, 1, 1);
    strokeWeight(3);
    stroke(0, 0, 0);
    line(cnt+5, 0, cnt+5, height);
    strokeWeight(1);
    stroke(255, 255, 255);
    line(cnt+1, 0, cnt+1, height);

    stroke(col[i]);
    line(prevTx[i], prevTy[i], tx, ty);
    prevTx[i]=tx;
    prevTy[i]=ty;
  }
  // 画面の右端まで描画したら再初期化
  if (cnt > width) {
    //initGraph();
    cnt=0;
    for (int i=0; i<NUM; i++) {
      prevTx[i]=0;
      prevTy[i]=0;
    }
  }
  //カウンタアップ
  cnt++;
}

//グラフの初期化
void initGraph() {
  background(255);
  noStroke();
  cnt = 0;
  // グラフ描画の線の色を定義
 
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
          dataPoint++;
        } else if (flag.equals("AIR")) {
          if (dataPoint==0) {
            //dataPoint is always 0 
            airData[dataPoint]=0.01*int(byteBuf);
            dataPoint++;
          } else {
            // convert to hect pascal
            airData[dataPoint]=float(int(byteBuf))/256/100;
            dataPoint++;
          }
        }
        byteBuf="";
        prevSeparator=i;
      }
    }
    i++;
  }

  byteBuf="";
  prevSeparator=0;
  flag="";
}