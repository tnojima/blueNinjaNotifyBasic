/**
 displayBlueNinjaNotify.js
 This is a sample program to show Accelerometer on the blueNinja.
 The data is transmitted from single blueNinja via BLE
 On blueNinja side, the sample program "HyouRowGan" is installed
 The official site of HyouRowGan is here -> https://blueninja.cerevo.com/ja/example/000_hyourowgan/
 
 usage:
 	node displayBlueNinjaNotify.js
 	Ctrl-C for quit
 	
 Before using this software, following commands may benecessary to install required libraries
   npm install noble
   npm install noble-device
   
 Related documents:
 	**Environment setup (1) installing nodebrew -> http://qiita.com/sinmetal/items/154e81823f386279b33c
 	**Environment setup (2) installing node.js that fit to noble (BLE communication module)-> http://blog.personal-factory.com/2017/01/26/exsample-ble-via-noble/
 	/// additional comment -> The page says v4.7.2 of Node.js is suitable for using noble. 
 	/// But I did not confirm it. Anyway, v4.7.2 environment works.
 	
 	**Handle binary data in Node.js-> 	https://mag.osdn.jp/13/04/09/193000/2
 	**Notify based data receive in BLE -> http://qiita.com/hotchpotch/items/9314c774da9eeaa56c12
 	**How to connect to a specific BLE device using device name -> http://botalab.m37.coreserver.jp/botalab.info/2016/08/19/noble-device-%E3%82%92%E4%BD%BF%E3%81%A3%E3%81%A6ble%E9%80%9A%E4%BF%A1%E3%81%A7genuinoarduino101%E3%81%8B%E3%82%89%E3%83%87%E3%83%BC%E3%82%BF%E3%82%92%E5%8F%97%E4%BF%A1%E3%81%99%E3%82%8B%EF%BC%8E/
 	
 	**How to Debug node.js -> https://mag.osdn.jp/13/04/16/080000
 	** How to connect multiple BLE devices -> http://qiita.com/ma2shita/items/f815f1f40cd10343017c
Originally mdified from blueNinjaSingle_ACC 2017/4/29 by Takuya Nojima

2017/5/9 Added UDP data transfer
  Added UDP data transfer functions to a certain server
  ** How to send data via UDP(Client.js) -> http://d.hatena.ne.jp/seinzumtode/20141130/1417308251

sample circuit for ADC, pin assignments
https://blueninja.cerevo.com/ja/example/003_adc12/

  TODO:
    add termination function
       -> Control+C is used to quit the program. But after this, both devices are also required to restart to connect again.
       add ADC value read function
       add GPIO value read function
       add GPIO value send function
*/
// Required library
var noble = require('noble');
var NobleDevice=require('noble-device');
var dgram = require('dgram');

var consoleout=1;
var consoleMTNout=0;
var consoleAIRout=0;
var consoleDINout=1;
var consoleADCout=1;

var udpout=1;
var udpMTNout=1;
var udpAIRout=1;
var udpDINout=1;
var udpADCout=1;

///////////////////////////////////////////////////////////////////////////
// Configuration of server to send data
var PORT = 8080;
var HOST = '127.0.0.1';

//////////////////////////////////////////////////
// Edit here to fit to each device
var idOrLocalName_R='BLE_SAMPLE00';
///////////////////////////////////////////////////////////////////////////
//BlueNinja Motion sensor Service (If you continue using BlueNinja, not necessary to edit here)
var UUID_SERCVICE_MSS= '00050000672711e5988ef07959ddcdfb';
var UUID_CHARACTERISTIC_MSSVALUE = '00050001672711e5988ef07959ddcdfb';
var UUID_CLIENT_CHARACTERISTIC_CONFIG = '0000290200001000800000805f9b34fb';

//BlueNinja AirPressure sensor Service (If you continue using BlueNinja, not necessary to edit here)
var UUID_SERCVICE_AIRP= '00060000672711e5988ef07959ddcdfb';
var UUID_CHARACTERISTIC_AIRPVALUE = '00060001672711e5988ef07959ddcdfb';

//BlueNinja AD Converter Service (If you continue using BlueNinja, not necessary to edit here)
var UUID_SERCVICE_ADC= '00040000672711e5988ef07959ddcdfb';
var UUID_CHARACTERISTIC_ADCVALUE = '00040001672711e5988ef07959ddcdfb';

//BlueNinja GPIO sensor Service (If you continue using BlueNinja, not necessary to edit here)
var UUID_SERCVICE_GPIO= '00010000672711e5988ef07959ddcdfb';
var UUID_CHARACTERISTIC_GPIOVALUE_READ = '00010001672711e5988ef07959ddcdfb';
var UUID_CHARACTERISTIC_GPIOVALUE_WRITE = '00010002672711e5988ef07959ddcdfb';

//////////////////////////////////////////////////////////////////////////
// Open UDP socket, defines data transfer function
var client = dgram.createSocket('udp4'); /// Create socket
// data send
var udpSend = function (message){
    client.send(message, 0, message.length, PORT, HOST, function(err, bytes) {
        if (err) throw err;
//    console.log('UDP message sent to ' + HOST +':'+ PORT);
    });
}
// close socket
var udpClose = function (){
    client.close();
}


///////////////////////////////////////////////////////////////////////////
/// BLE Notification handler
function BlueNinjaBasic(){
//  this.onBarometricPressureChangeBinded = this.onBarometricPressureChange.bind(this);
}

/// write to controll point
BlueNinjaBasic.prototype.writeMotionControllPoint = function(data, callback) {
  this.writeUInt8Characteristic(UUID_SERCVICE_MSS, UUID_CLIENT_CHARACTERISTIC_CONFIG, data, callback);
};

/// enable notify
BlueNinjaBasic.prototype.notifyMotionMeasument = function(callback) {
	  this.onMotionChangeBinded           = this.onMotionChange.bind(this);
  this.notifyCharacteristic(UUID_SERCVICE_MSS, UUID_CHARACTERISTIC_MSSVALUE, true, this.onMotionChangeBinded, callback);
};

/// disalbe notify
BlueNinjaBasic.prototype.unnotifyMotionMeasument = function(callback) {
  this.notifyCharacteristic(UUID_SERCVICE_MSS, UUID_CHARACTERISTIC_MSSVALUE, false, this.onMotionChangeBinded, callback);
};

/// get data when the data has changed
BlueNinjaBasic.prototype.onMotionChange = function(data) {
  this.convertMotionData(data, function(counter) {
    this.emit('motionChange', counter);
  }.bind(this));
};

/// read the measured data
BlueNinjaBasic.prototype.readMotionData = function(callback) {
  this.readDataCharacteristic(UUID_SERCVICE_MSS, UUID_CHARACTERISTIC_MSSVALUE, function(error, data) {
    if (error) {
      return callback(error);
    }

    this.convertMotionData(data, function(counter) {
      callback(null, counter);
    });
  }.bind(this));
};


/// data format transformation
BlueNinjaBasic.prototype.convertMotionData = function(data, callback) {
	var offset=0;
	// Gyro value in gr{xyz}/16.4 [deg/s]
	var grx = data.readInt16LE(offset);offset+=2;
	var gry = data.readInt16LE(offset);offset+=2;
	var grz = data.readInt16LE(offset);offset+=2;
	// Accelerometer value in  ac{xyz}/2048 [G]
	var acx = data.readInt16LE(offset);offset+=2;
	var acy = data.readInt16LE(offset);offset+=2;
	var acz = data.readInt16LE(offset);offset+=2;
     // Magneto value
	var mgx = data.readUInt16LE(offset);offset+=2;
	if(mgx>0xffff/2){
	    mgx=mgx-0xffff;
	}
	var mgy = data.readUInt16LE(offset);offset+=2;
	if(mgy>0xffff/2){
	    mgy=mgy-0xffff;
	}
	var mgz = data.readUInt16LE(offset);offset+=2;
	if(mgz>0xffff/2){
	    mgz=mgz-0xffff;
	}
	if((consoleout==1)&&(consoleMTNout==1)){
	    // actual Gyro value will be gr{xyz}/16.4 [deg/s]
	    console.log(this.localName+' in [deg/s]:'+grx/16.4+','+gry/16.4+','+grz/16.4+'');	
	    // actual aceleration value will be ac{xyz}/2048 [G]
	    console.log(this.localName+' in [G]:'+acx/2048+','+acy/2048+','+acz/2048);
	    // actual mag data
	    console.log(this.localName+' in [--]:'+mgx+','+mgy+','+mgz+'');	
	}
	if((udpout==1)&&(udpMTNout==1)){
	    udpSend('MTN,'+acx+','+acy+','+acz+','+grx+','+gry+','+grz+','+mgx+','+mgy+','+mgz+'\n');
	}
};

//=========================================================
// write to controll point
BlueNinjaBasic.prototype.writeAirPControllPoint = function(data, callback) {
  this.writeUInt8Characteristic(UUID_SERCVICE_AIRP, UUID_CLIENT_CHARACTERISTIC_CONFIG, data, callback);
};

/// enable notify
BlueNinjaBasic.prototype.notifyAirPMeasument = function(callback) {
	  this.onAirPChangeBinded           = this.onAirPChange.bind(this);
  this.notifyCharacteristic(UUID_SERCVICE_AIRP, UUID_CHARACTERISTIC_AIRPVALUE, true, this.onAirPChangeBinded, callback);
};

/// disalbe notify
BlueNinjaBasic.prototype.unnotifyAirPMeasument = function(callback) {
  this.notifyCharacteristic(UUID_SERCVICE_AIRP, UUID_CHARACTERISTIC_AIRPVALUE, false, this.onAirPChangeBinded, callback);
};

/// get data when the data has changed
BlueNinjaBasic.prototype.onAirPChange = function(data) {
  this.convertAirPData(data, function(counter) {
    this.emit('AirPChange', counter);
  }.bind(this));
};

/// read the measured data
BlueNinjaBasic.prototype.readAirPData = function(callback) {
  this.readDataCharacteristic(UUID_SERCVICE_AIRP, UUID_CHARACTERISTIC_AIRPVALUE, function(error, data) {
    if (error) {
      return callback(error);
    }

    this.convertAirPData(data, function(counter) {
      callback(null, counter);
    });
  }.bind(this));
};



/// data format transformation
BlueNinjaBasic.prototype.convertAirPData = function(data, callback) {
	var offset=0;
	// Temparature value in [0.01digC]
	var temp = data.readInt16LE(offset);offset+=2;
	//Air Pressure value in [1/256Pa]
	var airP = data.readInt32LE(offset);offset+=2;
	
	if((consoleout==1)&&(consoleAIRout==1)){
	    /// actual temparature will be temp*0.01[C]
	    console.log(this.localName+' in [digC]:'+temp*0.01);	
	    // actual air pressure will be airP/256[Pa]
	    console.log(this.localName+' in [Pa]:'+airP/256);
	}
	if((udpout==1)&&(udpAIRout==1)){
	    udpSend('AIR,'+temp+','+airP+'\n');
	}
};


//=========================================================
// write to controll point
BlueNinjaBasic.prototype.writeADCControllPoint = function(data, callback) {
  this.writeUInt8Characteristic(UUID_SERCVICE_ADC, UUID_CLIENT_CHARACTERISTIC_CONFIG, data, callback);
};

/// enable notify
BlueNinjaBasic.prototype.notifyADCMeasument = function(callback) {
	  this.onADCChangeBinded           = this.onADCChange.bind(this);
  this.notifyCharacteristic(UUID_SERCVICE_ADC, UUID_CHARACTERISTIC_ADCVALUE, true, this.onADCChangeBinded, callback);
};

/// disalbe notify
BlueNinjaBasic.prototype.unnotifyADCMeasument = function(callback) {
  this.notifyCharacteristic(UUID_SERCVICE_ADC, UUID_CHARACTERISTIC_ADCVALUE, false, this.onADCChangeBinded, callback);
};

/// get data when the data has changed
BlueNinjaBasic.prototype.onADCChange = function(data) {
  this.convertADCData(data, function(counter) {
    this.emit('ADCChange', counter);
  }.bind(this));
};

/// read the measured data
BlueNinjaBasic.prototype.readADCData = function(callback) {
  this.readDataCharacteristic(UUID_SERCVICE_ADC, UUID_CHARACTERISTIC_ADCVALUE, function(error, data) {
    if (error) {
      return callback(error);
    }

    this.convertADCData(data, function(counter) {
      callback(null, counter);
    });
  }.bind(this));
};

/// data format transformation
BlueNinjaBasic.prototype.convertADCData = function(data, callback) {
		var offset=0;
		var adc0 = data.readUInt16LE(offset);offset+=2;
		var adc1 = data.readUInt16LE(offset);offset+=2;
		var adc2 = data.readUInt16LE(offset);offset+=2;
		var adc3 = data.readUInt16LE(offset);offset+=2;
    	if((consoleout==1)&&(consoleADCout==1)){
    		console.log(Buffer(data).toString("hex"));
    	    console.log(this.localName+' in [ADC]:'+adc0+':'+adc1+':'+adc2+':'+adc3);
    	}
    	if((udpout==1)&&(udpADCout==1)){
    	    udpSend('ADC,'+adc0+','+adc1+','+adc2+','+adc3+'\n');
    	}
};

//=========================================================
// write to controll point
BlueNinjaBasic.prototype.writeGPIOControllPoint = function(data, callback) {
  this.writeUInt8Characteristic(UUID_SERCVICE_GPIO, UUID_CLIENT_CHARACTERISTIC_CONFIG, data, callback);
};

/// enable notify
BlueNinjaBasic.prototype.notifyGPIOMeasument = function(callback) {
	  this.onGPIOChangeBinded           = this.onGPIOChange.bind(this);
  this.notifyCharacteristic(UUID_SERCVICE_GPIO, UUID_CHARACTERISTIC_GPIOVALUE_READ, true, this.onGPIOChangeBinded, callback);
};

/// disalbe notify
BlueNinjaBasic.prototype.unnotifyGPIOMeasument = function(callback) {
  this.notifyCharacteristic(UUID_SERCVICE_GPIO, UUID_CHARACTERISTIC_GPIOVALUE_READ, false, this.onGPIOChangeBinded, callback);
};

/// get data when the data has changed
BlueNinjaBasic.prototype.onGPIOChange = function(data) {
  this.convertReceivedGPIOData(data, function(counter) {
    this.emit('GPIOChange', counter);
  }.bind(this));
};

/// read the measured data
BlueNinjaBasic.prototype.readGPIOData = function(callback) {
  this.readDataCharacteristic(UUID_SERCVICE_GPIO, UUID_CHARACTERISTIC_GPIOVALUE_READ, function(error, data) {
    if (error) {
      return callback(error);
    }

    this.convertReceivedGPIOData(data, function(counter) {
      callback(null, counter);
    });
  }.bind(this));
};

/// data format transformation
BlueNinjaBasic.prototype.convertReceivedGPIOData = function(data, callback) {
	var gpio_val = data.readUInt8(0);
	var gpio_16=gpio_val&0x01;gpio_val = gpio_val>>1;
	var gpio_17=gpio_val&0x01;gpio_val = gpio_val>>1;
	var gpio_18=gpio_val&0x01;gpio_val = gpio_val>>1;
	var gpio_19=gpio_val&0x01;gpio_val = gpio_val>>1;
   	if((consoleout==1)&&(consoleDINout==1)){
   	 console.log(this.localName+' in [GPIO_in]:'+gpio_16+':'+gpio_17+':'+gpio_18+':'+gpio_19);
   	}
   	if((udpout==1)&&(udpDINout==1)){
   	 udpSend('DIN,'+gpio_16+','+gpio_17+','+gpio_18+','+gpio_19+'\n');
   	}
};
var gpio_20, gpio_21, gpio_22, gpio_23;
/// data format transformation
BlueNinjaBasic.prototype.convertSendingGPIOData = function(data, callback) {
    var gpio_send_val=0x00;
    gpio_send_val += gpio_20; gpio_send_val<<1;
    gpio_send_val += gpio_21; gpio_send_val<<1;
    gpio_send_val += gpio_22; gpio_send_val<<1;
    gpio_send_val += gpio_23;
    gpio_send_val=(gpio_send_val << 4)&0xf0;
    
	data.writeUInt8(gpio_send_val);
	//console.log(this.localName+' in [GPIO_out]:'+gpio_send_val+':'+gpio_20+':'+gpio_21+':'+gpio_22+':'+gpio_23);

};

///////////////////////////////////////////////////////////////////////////

/// Scan BLE devices
noble.on('stateChange', function(state) {
  if (state === 'poweredOn') {
    noble.startScanning();
  } else {
    noble.stopScanning();
  }
});
///////////////////////////////////////////////////////////
//// Set Device for Right
var myDevice_R = function(device) {
	NobleDevice.call(this, device);
	this.localName = device.advertisement.localName;
};

/// This function is used to connect to a specific BLE device.
myDevice_R.is = function(device) {
	var localName = device.advertisement.localName;
	return (device.id === idOrLocalName_R || localName === idOrLocalName_R);
};

NobleDevice.Util.inherits(myDevice_R, NobleDevice);
NobleDevice.Util.mixin(myDevice_R, NobleDevice.DeviceInformationService);
NobleDevice.Util.mixin(myDevice_R, BlueNinjaBasic); /// Registering user-defined handling service


myDevice_R.discover(function(device) {
	console.log('discovered R: ' + device.localName);
  	device.on('disconnect', function() {
  		console.log('disconnected! R');
    	process.exit(0);
 	});
 	
 	
    device.connectAndSetUp(function(callback) {
    	console.log('connectAndSetUp R');
    	device.notifyMotionMeasument(function(counter){
    		console.log('notifyMotion');
    	});
    	device.notifyAirPMeasument(function(counter){
    		console.log('notifyAirP');
    	});
    	device.notifyADCMeasument(function(counter){
    		console.log('notifyADC');
    	});
    	device.notifyGPIOMeasument(function(counter){
    		console.log('notifyADC');
    	});

    });
});
/*
process.on('SIGINT', function() {
    console.log("Caught interrupt signal");
    if(myDevice_R.is){
    device.on('disconnect', function() {
  		console.log('disconnected! R');
    	process.exit(0);
 	});
}
});
*/

