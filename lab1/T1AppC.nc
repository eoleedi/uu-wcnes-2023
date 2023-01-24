configuration T1AppC
{
}
implementation
{
  components MainC, T1C as App;
  App -> MainC.Boot;
 
  components LedsC;
  App.Leds -> LedsC;
 
  components new TimerMilliC() as TimerAccel;
  App.TimerAccel -> TimerAccel;
 
  components new ADXL345C();
  App.XYZaxis -> ADXL345C.XYZ;
  App.AccelControl -> ADXL345C.SplitControl;
}