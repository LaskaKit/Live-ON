# Live ON - (ZivyObraz project for ePaper display) without ePaper display
The ZivyObraz project, one FW for all ePaper dipleys and in the web service environment you can design your own look and feel and the extras you want to display on your ePaper display.</br>
But we needed something a little bit different - firmware that queries the ZivyObraz server, finds the deep-sleep value, then executes the code and goes back to sleep and wakes up again in the time it found by querying ZivyObraz. And in addition, it also sends sensor data and battery voltage. And that's not all, microESP can switch up to 3 devices via GPIO (CH0 - GPIO7, CH1 - 1, CH2 - GPIO4).</br>
</br>
And this is what our modified FW is all about. Sending measured values from the connected sensor, options to control external circuit-device, measure the voltage of the connected battery and determine how long it takes for the chip to wake up again, measure and send the data.</br>
</br>
After uploading the FW to the ESP32-C3 board (in our microESP code), the Wi-Fi AP is started - just like the original project we use a WiFi manager (the wifi password is zivyobraz). Once the AP is running, you connect to the ESP32-C3 via mobile phone, tablet or laptop and enter the WiFi password and SSID of your home network.</br>
Then you enter the MAC address of the device on zivyobraz.eu and the microESP will send the temperature, humidity, pressure or CO2 concentration to the ZivyObraz - depending on the connected sensor, you can then use the measured data as you wish - for example, display it on the ePaper display. In addition, the code can control up to 3 other devices via GPIO.</br>
</br>
Supported sensors are:</br>
SHT40 - temperature and humidity</br>
BME280 - pressure, temperature and humidity</br>
SCD41 - CO2, temperature, humidity</br>
</br>
## Original project</br>
* Basic information can be found on the project website: https://zivyobraz.eu/</br>
* For specific commissioning information, see the documentation at: https://wiki.zivyobraz.eu/</br>
