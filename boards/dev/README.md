# Devices
## Button Sensor
## LED Indicator
## Mainsdetector
## Pulse sensor
## Relay
## Timer
A timer will run from zero and up. The interval is in seconds and will wrap every 388days = 33554432 seconds
The timer will keep counting - it will not automatically stop.

Example:
Set the above event to 180 seconds
Set the below event to 60 seconds.
Pair the above event with the reset action.

1. A timer is started
2. After 1 minute, a below event will fire.
3. After Another 2 minutes the above event will fire, and because of the timer
above event is tied to the restart event will start to count from 0 again.


### Get value
If requested, the time till expiration will be returned.


### Events
Above event:
  Event fired when timer > setup
  Attach this event to the reset action to have it reset countinously.

Below event:
  Event fired when timer > setup
  Set it to 0 to have it fire an event on reset.

Change event:
  Event fired when delta time >= setup
  Sets the amount of seconds between an event.
  E.g set the change event to 60, and the event will be fired once every minute.

### Actions
Stop counting:
This will pause the current events

Start counting:
This will continue from where it was

Reset:
This will reset the counter to 0 and start from there.

Direction: (TODO)
Default is up, but it can be changed setting this action.

# Pulse Counter
