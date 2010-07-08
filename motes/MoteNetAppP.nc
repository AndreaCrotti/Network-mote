//#include <IPDispatch.h>
//#include <lib6lowpan.h>
//#include <ip.h>

module MoteNetAppP{
    uses{
        interface Boot;

        interface SplitControl as SerialControl;
        //interface SplitControl as RadioControl;

        interface Timer<TMilli> as Timer;

        interface Send as SerialSend;
        // For forwarding data to the network
        //interface Ieee154Send as RadioSend;
        interface Packet as RadioPacket;

        interface Receive as SerialReceive;
        
        //interface IPAddress;

        //interface UDP as UDPRadio;
        
        interface Leds;
    }
}
implementation{
  message_t packet;

  event void Boot.booted() {
    call SerialControl.start();
  }
  event void SerialControl.startDone(error_t err) {
    call Leds.led2On();
    call Timer.startPeriodic(2000);
  }
  event void SerialControl.stopDone(error_t err) {
    call Leds.led2Off();
  }

  event void Timer.fired() {
    call SerialSend.send(&packet, 5);
  }

  event void SerialSend.sendDone(message_t* m, error_t error) {
  }
  event message_t* SerialReceive.receive(message_t* m, void* payload, uint8_t len) {
    call Leds.led1On();
    return m;
  }

}
