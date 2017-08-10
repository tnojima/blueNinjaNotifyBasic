class ADCDataWindow extends PApplet {
  PApplet parent;
  
  ADCDataWindow(PApplet _parent) {
    super();
    // set parent
    this.parent = _parent;
    //// init window
    try {
      java.lang.reflect.Method handleSettingsMethod =
        this.getClass().getSuperclass().getDeclaredMethod("handleSettings", null);
      handleSettingsMethod.setAccessible(true);
      handleSettingsMethod.invoke(this, null);
    } catch (Exception ex) {
      ex.printStackTrace();
    }
    
    PSurface surface = super.initSurface();
    surface.placeWindow(new int[]{0, 0}, new int[]{0, 0});
    
    this.showSurface();
    this.startSurface();
  }
  
void settings() {
    size(1024, 768);
  }

  int NUM=4;
  String[] label={"ADC0", "ADC1", "ADC2","ADC3" };
  float baseADC=512;              /// Base value for sensor data(Acceleration G)
  float ADCrange=1024;  /// Sensor data range

  /// the size of this min/max array  must be the same as "NUM"
  float[] sensors_max={baseADC+ADCrange/2.0, baseADC+ADCrange/2.0, baseADC+ADCrange/2.0, baseADC+ADCrange/2.0};
  float[] sensors_min={baseADC-ADCrange/2.0, baseADC-ADCrange/2.0, baseADC-ADCrange/2.0, baseADC-ADCrange/2.0};

  float[] prevTx=new float[NUM];
  float[] prevTy=new float[NUM];
  
  int cnt; //カウンター

  void setup() {
    background(255);
    noStroke();
    frameRate(FRAME_RATE);

    initGraph();
  }

  float tickTime(int cnt) {
    return (float)cnt/FRAME_RATE;
  }

  void draw() {
    // センサーの数だけ、グラフをプロット  
    for (int i = 0; i < NUM; i++) {

      float tx = map(cnt, 0, width, 0, width);
      float ty = map(adcData[i], sensors_min[i], sensors_max[i], height/NUM, 0)+height/NUM*i;

      //Draw data labels
      fill(0);
      text(label[i], 0, height/NUM*(i+0.5));
      text(adcData[i], 30, height/NUM*(i+0.5));

      // Draw moving clear line
      strokeWeight(3);
      stroke(0, 0, 0);
      line(cnt+5, 0, cnt+5, height);
      strokeWeight(1);
      stroke(255, 255, 255);
      line(cnt+1, 0, cnt+1, height);

      /// Draw Axis
      strokeWeight(1);
      stroke(0, 0, 0);
      // horizontal origin line
      line(0, height/NUM/2+height/NUM*i, width, height/NUM/2+height/NUM*i);
      //draw intermediate line between graphs
      line(0, height/NUM+height/NUM*i, width, height/NUM+height/NUM*i);

      // Draw values
      fill(col[i]);
      // put origin value of sensor data
      text((sensors_min[i]+sensors_max[i])/2, 0, height/NUM*(i+0.5)+15);
      //put minimum value of sensor data
      text(sensors_min[i], 0, height/NUM*(i+1));
      //put maximum value of sensor data;
      text(sensors_max[i], 0, height/NUM*i+15);

      // Draw data line by connecting previous data and the current data
      stroke(col[i]);
      line(prevTx[i], prevTy[i], tx, ty);
      prevTx[i]=tx;
      prevTy[i]=ty;

      // draw time info in SEC after late_cnt frames
      int late_cnt=20;
      if (tickTime(cnt-late_cnt)%5==0) {
        fill(0);
        text(tickTime(cnt-late_cnt), cnt-late_cnt, height/NUM/2+height/NUM*i);
        stroke(0, 0, 0);
        line(cnt-late_cnt, height/NUM/2+height/NUM*i-10, cnt-late_cnt, height/NUM/2+height/NUM*i+10);
      }
    }

    // 画面の右端まで描画したら再初期化
    if (cnt > width) {
      initGraph();
    }
    // Count UP
    cnt++;
  }

  //Initialize the Graph
  void initGraph() {
    cnt = 0;
    for (int i=0; i<NUM; i++) {
      prevTx[i]=0;
      prevTy[i]=0;
    }
  }
}