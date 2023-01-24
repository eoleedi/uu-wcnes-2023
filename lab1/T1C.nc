#include "printfZ1.h"
#include "ADXL345.h"

#define ERROR_x 80
#define ERROR_y 80
#define ERROR_z 80

#define LEDS_TICK 8
#define DETECT_TICK 100

module T1C @safe()
{
  uses interface Boot;
  uses interface Leds;
  uses interface Timer<TMilli> as TimerAccel;
  uses interface Read<adxl345_readxyt_t> as XYZaxis;
  uses interface SplitControl as AccelControl;  
 
}
implementation
{
  static int x,y,z;
  static int x_counter, y_counter, z_counter;
  event void Boot.booted()
  {
    printfz1_init();
    call AccelControl.start();
  }
 
  event void TimerAccel.fired()
  {
    call XYZaxis.read();
  }
 
  event void AccelControl.startDone(error_t err) {
  	printfz1("  +  Accelerometer Started\n");
    call TimerAccel.startPeriodic( DETECT_TICK );
  }
 
  event void AccelControl.stopDone(error_t err) {
    printfz1("Accelerometer Stopped\n");
  }
  event void XYZaxis.readDone(error_t result, adxl345_readxyt_t data){
    int led = 0;
    printfz1("X (%d) + Y (%d) + Z(%d) \r", data.x_axis, data.y_axis, data.z_axis);
    if (x != data.x_axis && abs(x - data.x_axis) > ERROR_x){
      led += 1;
      x_counter = LEDS_TICK;
    }
    else if(x_counter > 0){
      x_counter --;
      led += 1;
    }

    if (y != data.y_axis && abs(y - data.y_axis) > ERROR_y){
      led += 2;
      y_counter = LEDS_TICK;
    }
    else if(y_counter > 0){
      y_counter --;
      led += 2;
    }

    if (z != data.z_axis && abs(z - data.z_axis) > ERROR_z){
      led += 4;
      z_counter = LEDS_TICK;
    }
    else if(z_counter > 0){
      z_counter --;
      led += 4;
    }
    x = data.x_axis;
    y = data.y_axis;
    z = data.z_axis;
    call Leds.set(led);
  }
}