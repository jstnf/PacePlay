#include "math.h"
#include "Wire.h" // This library allows you to communicate with I2C devices.
#include <SoftwareSerial.h>

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data

const int hist_len = 100;
const int16_t overflow_correction = 19250;

// previous data for calculating dx, dy, dz
int16_t accelerometer_px[hist_len];
int16_t accelerometer_py[hist_len];
int16_t accelerometer_pz[hist_len];

const int delay_times_len = 5;
int delay_times[delay_times_len];

int16_t accel_max, accel_min;

int iterations = 0;
int last_upper_wave_entry = -1;

char tmp_str[7]; // temporary variable used in convert function

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

SoftwareSerial HC05Module(3, 3);

void move_data() {
  for (int i = hist_len - 1; i > 0; i--) {
    accelerometer_px[i] = accelerometer_px[i - 1];
    accelerometer_py[i] = accelerometer_py[i - 1];
    accelerometer_pz[i] = accelerometer_pz[i - 1];
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  // HC05Module.begin(9600);
}

int16_t get_max_deriv() {
  int16_t dx = abs(accelerometer_x - accelerometer_px[0]);
  int16_t dy = abs(accelerometer_y - accelerometer_py[0]);
  int16_t dz = abs(accelerometer_z - accelerometer_pz[0]);
  return max(max(dx, dy), dz);
}

int16_t average(int16_t* data, int16_t len) {
  int16_t total = 0;
  for (int16_t i = 0; i < len && i < iterations; i++) {
    total += data[i];
  }
  return total / len;
}

int16_t get_smooth_max_deriv() {
  int16_t smooth_dx = abs(accelerometer_x - average(accelerometer_px, hist_len));
  int16_t smooth_dy = abs(accelerometer_y - average(accelerometer_py, hist_len));
  int16_t smooth_dz = abs(accelerometer_z - average(accelerometer_pz, hist_len));
  return max(smooth_dx, max(smooth_dy, smooth_dz));
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers

  // prep historical data
  move_data();
  accelerometer_px[0] = accelerometer_x;
  accelerometer_py[0] = accelerometer_y;
  accelerometer_pz[0] = accelerometer_z;
  
  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
  
  // print out data
//  String output = String("aX = ");
//  output.concat(convert_int16_to_str(accelerometer_x));
//  output.concat(" | aY = ");
//  output.concat(convert_int16_to_str(accelerometer_y));
//  output.concat(" | aZ = ");
//  output.concat(convert_int16_to_str(accelerometer_z));

//  String output = String("d(aX):");
//  output.concat(convert_int16_to_str(accelerometer_x - accelerometer_px[0]));
//  output.concat(",d(aY):");
//  output.concat(convert_int16_to_str(accelerometer_y - accelerometer_py[0]));
//  output.concat(",d(aZ):");
//  output.concat(convert_int16_to_str(accelerometer_z - accelerometer_pz[0]));

  String var_name = String("dmax:");
  int16_t data = get_smooth_max_deriv();

  
  // Set max and min
  if (iterations == 0) {
    accel_max = data;
    accel_min = data;
  } else {
    if (data > accel_max) accel_max = data;
    if (data < accel_min) accel_min = data;
  }

  String output = String(var_name);
  output.concat(convert_int16_to_str(data));
  
  // HC05Module.write(output.c_str());
  Serial.println(output);
  
  // Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
  // Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
  // Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  // Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);
  // Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
  // Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
  // Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
  // Serial.println();
  
  // delay
  iterations++;
  delay(10);
}
