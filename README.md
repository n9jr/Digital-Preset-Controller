![DPC_HomeScreen](https://github.com/user-attachments/assets/26b13c4f-0b4a-4fee-811e-bba0a968de73)
This is auxillary controller for the Yaesu G-800, G-1000 and G-2800 series of antenna rotors.  It extends the functionality by adding the following features:
- Digitral display of rotor direction.
- Two banks of 8 preset memories.
- Keypad entry of direction.
- A slider bar for slide and go entry.
- A configurable soft start and stop zones to lessen hardware wear.
- Configurable rotor speeds.
- Customizable user preferences in the system memus

The controller is based on the Arduino Nano and uses a 3.5" Nextion display.  To program the Nano you will need to install the Arduino IDE.  You will also need to add the EasyNextion library through the Library Manager in the IDE.  The EasyNextion library as installed is limited to 50 functions.  I have modified two of the library files trigger.h and calltriggers.cpp and you need to replace the origionals with these.

Under Windows you will find the files at c:\users\<name>\Documents\Arduino\Libraries\Easy_Nextion_Library\src
For MacOS it is located under Users/<user>/Documents/Arduino/Libraries/Easy_Nextion_Library/src
