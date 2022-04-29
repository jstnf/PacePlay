#include <LiquidCrystal.h>

#include "math.h"
#include "Wire.h" // This library allows you to communicate with I2C devices.
#include <SoftwareSerial.h>

/* ------ */
/* GY 521 */
/* ------ */

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data

const int hist_len = 200;

// accel data for calculating dax, day, daz
int accelerometer_px[hist_len];
int accelerometer_py[hist_len];
int accelerometer_pz[hist_len];

// gyro data for calculating dgx, dgy, dgz
int gyro_px[hist_len];
int gyro_py[hist_len];
int gyro_pz[hist_len];

// used to get 2nd derivative
int last_damax, last_dgmax;
int pddamax[hist_len];
int pddgmax[hist_len];
int plast_da_avg[hist_len];

char tmp_str[7]; // temporary variable used in convert function
char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

/* ------ */


// LED DISPLAY
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


// Pedometer
const int rise_fall_len = 30;
unsigned long ay_rise_intervals[rise_fall_len];
unsigned long ay_fall_intervals[rise_fall_len];
unsigned long ay_last_rise_time = 0;
unsigned long ay_last_fall_time = 1;
float bpm = 0.0;

/*
unsigned long ddamax_rise_intervals[rise_fall_len];
unsigned long ddamax_fall_intervals[rise_fall_len];
unsigned long ddamax_last_rise_time = 0;
unsigned long ddamax_last_fall_time = 1;
unsigned long ddgmax_rise_intervals[rise_fall_len];
unsigned long ddgmax_fall_intervals[rise_fall_len];
unsigned long ddgmax_last_rise_time = 0;
unsigned long ddgmax_last_fall_time = 1;
*/

// Time-keeping
long iterations = 0;

// Bluetooth and button
bool pressed = false;
int button_state = 0;
const int BUTTON_INPUT = 8;
const int HC05_TX = 14;
const int HC05_RX = 15;
SoftwareSerial HC05Module(HC05_RX, HC05_TX);

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  // LED
  lcd.begin(16, 2);

  // Bluetooth and button
  pinMode(BUTTON_INPUT, INPUT);
  HC05Module.begin(9600);
}



void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers

  /*
  // prep historical data
  move_data();
  accelerometer_px[0] = accelerometer_x;
  accelerometer_py[0] = accelerometer_y;
  accelerometer_pz[0] = accelerometer_z;
  gyro_px[0] = gyro_x;
  gyro_py[0] = gyro_y;
  gyro_pz[0] = gyro_z;
  */
  
  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

  // Let's simplify this process - only use accelerometer y
  if (ay_last_rise_time > ay_last_fall_time) { 
    // Look for the next fall and if it fell, record the time and interval
    if (accelerometer_y < 0) {
      unsigned long now = millis();
      move_data_ulong(ay_fall_intervals, rise_fall_len);
      ay_fall_intervals[0] = now - ay_last_fall_time;
      ay_last_fall_time = now;
    }
  } else {
    // Look for the next rise and if it rose, record the time and interval
    if (accelerometer_y > 0) {
      unsigned long now = millis();
      move_data_ulong(ay_rise_intervals, rise_fall_len);
      ay_rise_intervals[0] = now - ay_last_rise_time;
      ay_last_rise_time = now;
    }
  }

  unsigned long interval_ms = average_ulong(ay_rise_intervals, rise_fall_len);
  bpm = (60000.0 / (float) interval_ms) * 2; // multiply by 2 because we are detecting arm swing - arm swings once a step

  // LCD Display
  String line1 = String("BPM: ");
  line1.concat(bpm);
  line1.concat("   ");
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  if (bpm <= 110) {
    lcd.print("Walking");
  } else if (bpm <= 155) {
    lcd.print("Jogging");
  } else {
    lcd.print("Running");
  }

  // Bluetooth and button
  button_state = digitalRead(BUTTON_INPUT);
  if (pressed) {
    if (button_state == LOW) {
      // Depress
      pressed = false;
    }
  } else {
    if (button_state == HIGH) {
      pressed = true;

      String output = "s";
      output.concat(bpm);
      output.concat("e");
      
      Serial.println(output);
      HC05Module.print(output); // Do bluetooth communication
    }
  }
  
  // Delay
  iterations++;
  delay(50);
  
  // print out data
  /*
  String output = String("aX: ");
  output.concat(convert_int16_to_str(accelerometer_x));
  output.concat(",aY: ");
  output.concat(convert_int16_to_str(accelerometer_y));
  output.concat(",aZ: ");
  output.concat(convert_int16_to_str(accelerometer_z));
  */

  /*
  int damax = get_accel_max_deriv();
  int dgmax = get_gyro_max_deriv();

  int ddamax = 0;
  int ddgmax = 0;
  //int dda_avg = 0;
  if (iterations != 0) {
    ddamax = abs(damax - last_damax);
    ddgmax = abs(dgmax - last_dgmax);
  }

  last_damax = damax;
  last_dgmax = dgmax;
  pddamax[0] = ddamax;
  pddgmax[0] = ddgmax;

  int avg_ddamax = average(pddamax, hist_len);
  int avg_ddgmax = average(pddgmax, hist_len);
  */

  /*
  if (ddamax_last_rise_time > ddamax_last_fall_time) { 
    // Look for the next fall and if it fell, record the time and interval
    if (ddamax < avg_ddamax) {
      unsigned long now = millis();
      move_data_ulong(ddamax_fall_intervals, rise_fall_len);
      ddamax_fall_intervals[0] = now - ddamax_last_fall_time;
      ddamax_last_fall_time = now;
    }
  } else {
    // Look for the next rise and if it rose, record the time and interval
    if (ddamax > avg_ddamax) {
      unsigned long now = millis();
      move_data_ulong(ddamax_rise_intervals, rise_fall_len);
      ddamax_rise_intervals[0] = now - ddamax_last_rise_time;
      ddamax_last_rise_time = now;
    }
  }

  if (ddgmax_last_rise_time > ddgmax_last_fall_time) { 
    // Look for the next fall and if it fell, record the time and interval
    if (ddgmax < avg_ddgmax) {
      unsigned long now = millis();
      move_data_ulong(ddgmax_fall_intervals, rise_fall_len);
      ddgmax_fall_intervals[0] = now - ddgmax_last_fall_time;
      ddgmax_last_fall_time = now;
    }
  } else {
    // Look for the next rise and if it rose, record the time and interval
    if (ddgmax > avg_ddgmax) {
      unsigned long now = millis();
      move_data_ulong(ddgmax_rise_intervals, rise_fall_len);
      ddgmax_rise_intervals[0] = now - ddgmax_last_rise_time;
      ddgmax_last_rise_time = now;
    }
  }
  

  // Attempt to compute our bpm using all of our known values!
  unsigned long interval_avgs[4] = {
    average_ulong(ddamax_rise_intervals, rise_fall_len),
    average_ulong(ddamax_fall_intervals, rise_fall_len),
    average_ulong(ddgmax_rise_intervals, rise_fall_len),
    average_ulong(ddgmax_fall_intervals, rise_fall_len)
  };
  unsigned long interval_ms = average_ulong(interval_avgs, 4);

  // Divide millisecond interval by milliseconds in minute to get bpm
  bpm = (60000.0 / (float) interval_ms);
  */
  

  /*
  String output = String("damax:");
  output.concat(ddamax);
  output.concat(",dgmax:");
  output.concat(ddgmax);
  */

  /*
  String output = String("ddamax:");
  output.concat(ddamax);
  output.concat(",ddgmax:");
  output.concat(ddgmax);
  output.concat(",avg_ddamax:");
  output.concat(avg_ddamax);
  output.concat(",avg_ddgmax:");
  output.concat(avg_ddgmax);
  output.concat(",aY:");
  output.concat(accelerometer_y);
  */
  
  //String output = String("bpm:");
  //output.concat(interval_ms);
  
  // HC05Module.write(output.c_str());
  // Serial.println(output);
  
  // Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
  // Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
  // Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  // Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);
  // Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
  // Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
  // Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
  // Serial.println();
}

void move_data() {
  for (int i = hist_len - 1; i > 0; i--) {
    accelerometer_px[i] = accelerometer_px[i - 1];
    accelerometer_py[i] = accelerometer_py[i - 1];
    accelerometer_pz[i] = accelerometer_pz[i - 1];
    gyro_px[i] = gyro_px[i - 1];
    gyro_py[i] = gyro_py[i - 1];
    gyro_pz[i] = gyro_pz[i - 1];
    pddamax[i] = pddamax[i - 1];
    pddgmax[i] = pddgmax[i - 1];
  }
}

void move_data_ulong(unsigned long *data, int len) {
  for (int i = len - 1; i > 0; i--) {
    data[i] = data[i - 1];
  }
}

int average(int data[], int len) {
  long total = 0;
  for (int i = 0; i < len && i < iterations; i++) {
    total = total + data[i];
  }
  return (int) (total / len);
}

unsigned long average_ulong(unsigned long data[], int len) {
  unsigned long total = 0;
  for (int i = 0; i < len && i < iterations; i++) {
    total = total + data[i];
  }
  return total / len;
}

int get_accel_max_deriv() {
  int dx = abs(accelerometer_x - accelerometer_px[0]);
  int dy = abs(accelerometer_y - accelerometer_py[0]);
  int dz = abs(accelerometer_z - accelerometer_pz[0]);
  return dy;
  // return max(max(dx, dy), dz);
}

int get_accel_smooth_max_deriv() {
  int smooth_dx = abs(((int) accelerometer_x) - average(accelerometer_px, hist_len));
  int smooth_dy = abs(((int) accelerometer_y) - average(accelerometer_py, hist_len));
  int smooth_dz = abs(((int) accelerometer_z) - average(accelerometer_pz, hist_len));
  return smooth_dy;
  //return max(smooth_dx, max(smooth_dy, smooth_dz));
}

int get_gyro_max_deriv() {
  int dx = abs(gyro_x - gyro_px[0]);
  int dy = abs(gyro_y - gyro_py[0]);
  int dz = abs(gyro_z - gyro_pz[0]);
  return dy;
  //return max(max(dx, dy), dz);
}

int get_gyro_smooth_max_deriv() {
  int smooth_dx = abs(((int) gyro_x) - average(gyro_px, hist_len));
  int smooth_dy = abs(((int) gyro_y) - average(gyro_py, hist_len));
  int smooth_dz = abs(((int) gyro_z) - average(gyro_pz, hist_len));
  return smooth_dy;
  //return max(smooth_dx, max(smooth_dy, smooth_dz));
}
