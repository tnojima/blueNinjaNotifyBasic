/* //<>// //<>//
 * Arduino - Processingシリアル通信
 * センサーの値をグラフにプロット
 * Processing側サンプル
 http://yoppa.org/tau_bmaw13/4790.html
 */
import processing.serial.*;
import hypermedia.net.*;

final int PORT = 8080;

int NUM = 11; //センサーの数
UDP udp;
String log;

float[] mtnData=new float[6];
int[] mgData=new int[3];
float[] airData=new float[2];

int[] sensors_min= {-16,-16,-16,-2000,-2000,-2000,-500,-500,-500,  20,1000};
int[] sensors_max={  16, 16,  16,  2000,  2000,  2000, 500, 500, 500,   50,1200};
float[] prevTx=new float[NUM];
float[] prevTy=new float[NUM];
String[] label={"GX","GY","GZ","GRX","GRY","GRZ","MGX","MGY","MGZ","TEMP","AIRP"};

int cnt; //カウンター

// グラフの線の色を格納
color[] col = new color[NUM];

void setup() {
  //画面設定
  size(800, 400);
  frameRate(60);

  udp = new UDP(this, PORT);
  udp.listen(true);

  //グラフ初期化
  initGraph();
}

void draw() {
  // センサーの数だけ、グラフをプロット  
  float data=0;
  for (int i = 0; i < NUM; i++) {
    if(i<6){
      data=mtnData[i];
    }else if(i<9){
      data=float(mgData[i-6]);
    }else if(i<11){
      data=airData[i-9];
    }

    float tx = map(cnt, 0, width, 0, width);
    float ty = map(data, sensors_min[i], sensors_max[i], height/NUM, 0)+height/NUM*i;
    fill(0);
    text(label[i],0,height/NUM*(i+0.5));
    //ellipse(tx, ty, 1, 1);
    strokeWeight(3);
    stroke(0,0,0);
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
  col[0] = color(255, 127, 31);
  col[1] = color(31, 255, 127);
  col[2] = color(127, 31, 255);
  col[3] = color(31, 127, 255);
  col[4] = color(127, 255, 31);
  col[5] = color(127,127,0);
  col[6] = color(255, 0, 0);
  col[7] = color(0, 255, 0);
  col[8] = color(0, 0, 255);
  col[9] = color(127, 0, 0);
  col[10] = color(0, 127, 0);
}


void receive(byte data[]) {
  int i=0;
  int prevSeparator=0;
  int j;
  int dataPoint=0;
  String  flag="";
  String  byteBuf="";
  byte separator=',';
  boolean first_separator_has_come=false;
  while (data[i] != '\n') {
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
          if(dataPoint<3){
            mtnData[dataPoint]=float(int(byteBuf))/2048;
          }else if(dataPoint<6){
            mtnData[dataPoint]=float(int(byteBuf))/16.4;
          }else{
            mgData[dataPoint-6]=int(byteBuf);
          }
          dataPoint++;
        } else if (flag.equals("AIR")) {
          //dataPoint is always 0 
          airData[dataPoint]=0.01*int(byteBuf);
          dataPoint++;
        }
        byteBuf="";
        prevSeparator=i;
      }
    }
    i++;
  }
  if (data[i]=='\n') {
    for (j=prevSeparator+1; j<i-1; j++) {
      byteBuf+=str(char(data[j]));
    }
    byteBuf+=str(char(data[i-1]));
  }
  if (flag.equals("MTN")) {
    mgData[dataPoint-6]=int(byteBuf);
    dataPoint++;
  } else if (flag.equals("AIR")) {
    airData[dataPoint]=float(int(byteBuf))/256/100;
    dataPoint++;
  }

  byteBuf="";
  prevSeparator=0;
  flag="";
}