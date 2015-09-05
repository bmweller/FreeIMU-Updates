        
//Comment these lines out for ARM processors such as DUE and Teensy 3.1
  //#include <EEPROM.h>
  //#include <stlport.h>
  //#include <iostream>
  //#include <Eigen30.h>

//Comment this line out for AVR boards such as ARduino Mega or Mircoduino
  #include <EigenArm.h>

#include <ExtendedKalman.h>

#include <struct_calibratedData.h>
#include <struct_euler.h>
#include <Quaternion.h>

s_calibratedData calibratedData;
s_euler orientation;
QuaternionClass quaternion;

//    KalmanClass kalmanPhi_, kalmanPsi_;
ExtendedKalmanClass EKF;


#define Has_LSM303 0
#define HAS_GPS 0

float const PI_F = 3.14159265F;
int16_t raw_values[11];
float ypr[3]; // yaw pitch roll
char str[128];
float val[11];
float val_array[18]; 
unsigned long lastUpdate, now; 			// sample period expressed in milliseconds
float dt;

// Set the FreeIMU object and LSM303 Compass
FreeIMU my3IMU = FreeIMU();


//The command from the PC
char cmd, tempCorr;


void setup() {
  Serial.begin(115200);
  Wire.begin();
  

  
  //#if HAS_MPU6050()
  //    my3IMU.RESET();
  //#endif
	
  my3IMU.init(true);
  
  delay(200);
  my3IMU.initGyros();
  Serial.println("FreeIMU initialized");
  // LED
  pinMode(13, OUTPUT);
}

void loop() {

        my3IMU.getValues(val);
        //sprintf(str, "%f,%f,%f,%f,%f,%f,%f,%f,%f,", val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8]);
        //Serial.print(str);
        //Serial.println("-------");
        
	now = micros();
        dt = ((now - lastUpdate) / 1000000.0);
           
        calibratedData.x =  (val[0]);
        calibratedData.y =  (val[1]);
        calibratedData.z =  (val[2]); 
        calibratedData.p =  (val[3]);
        calibratedData.q =  (val[4]);
        calibratedData.r =  (val[5]);
        calibratedData.magx =  (val[6]);
        calibratedData.magy =  (val[7]);
        calibratedData.magz =  (val[8]);
        calibratedData.q = -calibratedData.q;
        //calibratedData.temp = (my3IMU.getBaroTemperature());
        //calibratedData.pressure = (my3IMU.getBaroPressure());
        //calibratedData.altitude = ((pow(((1013.5) / calibratedData.pressure), 1/5.257) - 1.0) * (calibratedData.temp + 273.15)) / 0.0065;
      
        //Serial.print(quaternion.w);Serial.print(",  ");
        //Serial.print(quaternion.x);Serial.print(",  "); Serial.print(quaternion.y); Serial.print(",  ");
        //Serial.print(quaternion.z); Serial.println(); 
        //Serial.println(dt,4);
        
        //if(dt/100 < 0.06) {
	    quaternion = EKF.predict(&calibratedData, dt);
        //}
        my3IMU.getValues(val);
        quaternion = EKF.update(&calibratedData, dt);
        
        //Serial.print(quaternion.x);Serial.print(",  "); Serial.print(quaternion.y); Serial.print(",  ");
        //Serial.print(quaternion.z); Serial.println();
        
        //getYawPitchRollDeg(&quaternion);
        getYawPitchRoll180(&quaternion);
        lastUpdate = now; 	
        //SerialPrintFloatArr(val_array,18);
} 

    //******************************************************/




void getYawPitchRollDeg(QuaternionClass* q) {
  //float q[4]; // quaternion
  //float val[11];
  float gx, gy, gz; // estimated gravity direction
  //getQ(q, val);
  
  gx = 2 * (q->x*q->z - q->w*q->y);
  gy = 2 * (q->w*q->x + q->y*q->z);
  gz = q->w*q->w - q->x*q->x - q->y*q->y + q->z*q->z;
  
  ypr[0] = (180/pi) * atan2(2 * q->x * q->y - 2 * q->w * q->z, 2 * q->w*q->w + 2 * q->x * q->x - 1);
  ypr[1] = -(180/pi) * atan(gx / sqrt(gy*gy + gz*gz));
  ypr[2] = (180/pi) * atan(gy / sqrt(gx*gx + gz*gz));

  //Serial.print(ypr[1]); Serial.print(","); Serial.print(ypr[2]);
  //Serial.print(","); Serial.print(ypr[0]); Serial.println('\n');
    Serial.print(F("Orientation: "));
    Serial.print(ypr[0]);
    Serial.print(F(" "));
    Serial.print(ypr[1]);
    Serial.print(F(" "));
    Serial.print(ypr[2]);
    Serial.println(F(""));  
}


void getYawPitchRoll180(QuaternionClass* q) {
	float gx, gy, gz;		// estimated gravity direction

	gx = 2 * (q->x*q->z - q->w*q->y);
	gy = 2 * (q->w*q->x + q->y*q->z);
	gz = q->w*q->w - q->x*q->x - q->y*q->y + q->z*q->z;

	//calculating yaw
	ypr[0] = (180./PI)*atan2(2 * q->x * q->y - 2 * q->w * q->z, 2 * q->w*q->w + 2 * q->x * q->x - 1);	
	//ypr[0] = val[9];
	if(ypr[0] > 180.) ypr[0] = ypr[0] - 360.;
	ypr[0] = ypr[0] * 0.0174532925;
	
	//calculating Pitch
	//Serial.print(gx); Serial.print("       "); Serial.print(gz); Serial.print("       ");
	if(gx > 0 && gz > 0) {
		ypr[1] = atan(gx / sqrt(gy*gy + gz*gz));
	} else if(gx > 0 && gz <= 0) {
		ypr[1] = PI - atan(gx / sqrt(gy*gy + gz*gz));
	} else if(gx < 0 && gz < 0) {
		ypr[1] = (-PI - atan(gx / sqrt(gy*gy + gz*gz)));
	} else  {
		ypr[1] =  atan(gx / sqrt(gy*gy + gz*gz));
	}
	
	//Calculating Roll1
        ypr[2] = atan(gy / sqrt(gx*gx + gz*gz));
        
        ypr[0]= ypr[0]*180./PI;
        ypr[1]= ypr[1]*180./PI;
        ypr[2]= ypr[2]*180./PI;
        
        //adjust pitch so its 0-360 deg
        ypr[1] = -ypr[1];
        if(ypr[1] < 0) ypr[1] = 360. + ypr[1];
   
    Serial.print(F("Orientation: "));
    Serial.print(ypr[0]);
    Serial.print(F(" "));
    Serial.print(ypr[1]);
    Serial.print(F(" "));
    Serial.print(ypr[2]);
    Serial.println(F(""));
}

