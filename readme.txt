Profielwerkstuk Robot
---------------------
Code for robot implemented as part of high school final project. The robot has built using the STM32F4 Discovery platform, and the goal was to create a basic remote-controlled robot from scratch. The report is written in Dutch.

=== INSTALLATION ===
1. Follow the instructions here: http://cu.rious.org/make/stm32f4-discovery-board-with-linux/ (dead link, archived version at https://web.archive.org/web/20130517110141/http://cu.rious.org/make/stm32f4-discovery-board-with-linux/)
2. Run: make
3. Run: st-flash write build/pws.bin 0x8000000
4. Restart by pressing the RESET-button.

=== CHANGELOG ===
1.0: first public version (June 2013)
	- reading of sensors
  - sending commands over WiFi
	- basic robot control in time units

=== COPYRIGHT ===
(c) 2012-2013 Koen Wolters and Erik Kooistra
