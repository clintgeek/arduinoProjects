# README #

### TripCase test equipment cabinet lock ###

* Provides access control for test equipment
* Uses Arduino Nano as controller
* Uses 2x18650 Li-ion batteries as power source
* Is completely reversible
* Key lock is still functional 

### How do I get set up? ###

* Modify sketch as necessary and load using Arduino IDE
* Configuration is handled at the top of the sketch for most items
* Dependencies: keypad and servo libraries available through Arduino IDE

### Usage Instructions ###

#### To set passcode ####
* Default Passcode is: 1234, it will reset to this after power cycle
* To change passcode, unlock cabinet, hold "4" for 1 second then input new 4 digit code
#### Normal Use ####
* Input passcode and open drawer within 10 seconds
* Alarm will sound if drawer is open for more than one minute, close drawer to reset
* Drawer will re-lock automatically when closed
#### Batteries ####
* When batteries are low a repeating high-low alarm will sound
* Swap batteries with charged batteries in drawer and give the depleted ones to Proctor for charging
* Reset passcode after battery swap as it will reset to 1234 (see above)
* Battery Installation:

		       ----------------------
		      |                      |
		      | (-)              (+) |
		      |                      |
		======|----------------------
		      |                      |
		      | (+)              (-) |
		      |                      |
		       ----------------------

### Contribution guidelines ###

* Please follow best practices and preferred styles as set by the Arduino community.

### Who do I talk to? ###

* Oliver Acevedo (oliver.acevedo@sabre.com)
* Clint Crocker (clint.crocker@sabre.com)