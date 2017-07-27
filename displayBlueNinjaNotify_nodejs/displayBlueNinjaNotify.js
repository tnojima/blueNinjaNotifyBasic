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

  TODO:
    add termination function
       -> Control+C is used to quit the program. But after this, both devices are also required to restart to connect again.
*/
// Required library
var noble = require('noble');
var NobleDevice=require('noble-device');
var dgram = require('dgram');

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

//////////////////////////////////////////////////////////////////////////
// Open UDP socket, defines data transfer function
var client = dgram.createSocket('udp4'); /// Create socket
// data send
var udpSend = function (message){
    client.send(message, 0, message.length, PORT, HOST, function(err, bytes) {
        if (err) throw err;
    console.log('UDP message sent to ' + HOST +':'+ PORT);
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
	// Gyro value in [degree/s]
	var grx = data.readInt16LE(offset)/16.4;offset+=2;
	var gry = data.readInt16LE(offset)/16.4;offset+=2;
	var grz = data.readInt16LE(offset)/16.4;offset+=2;
	console.log(this.localName+' in [deg/s]:'+grx+','+gry+','+grz+'');	
	// Accelerometer value in [G]
	var acx = data.readInt16LE(offset)/2048;offset+=2;
	var acy = data.readInt16LE(offset)/2048;offset+=2;
	var acz = data.readInt16LE(offset)/2048;offset+=2;
	var acG=Math.sqrt(acx*acx+acy*acy+acz*acz);
	console.log(this.localName+' in [G]:'+acx+','+acy+','+acz+','+acG);
    //udpSend(this.localName+' in [G]:'+acx+','+acy+','+acz+'');
       // Magneto value
	var mgx = data.readUInt16LE(offset);offset+=2;
	var mgy = data.readUInt16LE(offset);offset+=2;
	var mgz = data.readUInt16LE(offset);offset+=2;
	console.log(this.localName+' in [?]:'+mgx+','+mgy+','+mgz+'');	
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
  this.readDataCharacteristic(UUID_SERCVICE_AIRP, UUID_CHARACTERISTIC_MSSVALUE, function(error, data) {
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
	console.log(this.localName+' in [0.01digC]:'+temp+':'+temp*0.01);	
	//Air Pressure value in [1/256Pa]
	var airP = data.readInt32LE(offset);offset+=2;
	console.log(this.localName+' in [1/256Pa]:'+airP+':'+airP/256);

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
 	
 	device.on('motionChange', function(data){
 		console.log("update measurement R:"+data);
 	});
 	
    device.connectAndSetUp(function(callback) {
    	console.log('connectAndSetUp R');
    	device.notifyMotionMeasument(function(counter){
    		console.log('notifyMotion');
    	});
    	device.notifyAirPMeasument(function(counter){
    		console.log('notifyAirP');
    	});

    });
});


