# Live ON - (Živý Obraz) bez ePaperu

Kdo by neznal projekt Živý Obraz, jednotný FW pro všechny ePaper dipleje a ve webovém prostředí služby si pak navrhneš vlastní vzhled a doplňky, které chceš zobrazit na ePaper displeji.</br>
My jsme ale potřebovali něco trochu jiného - firmware, který se dotáže na server ŽivýObraz, zjistí hodnotu deep-sleep, tedy časový údaj za jak dlouho se má deska opět probudit, poté provede daný kód a opět se uspí a probudí za čas, který zjistil dotazem na ŽivýObraz. A kromě toho také pošle data z čidel a napětí baterie. A to není všechno, microESP může spínat až 3 zařízení přes GPIO (CH0 - GPIO7, CH1 - 1, CH2 - GPIO4).</br>
</br>
A o tom je právě námi upravený FW. Odeslání naměřených hodnot z připojeného čidla, možnosti řídit externí obvod-zařízení, měřit napětí připojené baterie a zjištění, za jak dlouho se má čip opět probudit, změřit a odeslat data.</br>
</br>
Po nahrání FW do ESP32-C3 desky (v našem kódu microESP) se spustí Wi-Fi AP - stejně jako původní projekt používáme WiFi manager (wifi heslo je zivyobraz). Po spuštění AP se na ESP32-C3 připojíš přes mobilní telefon, tablet nebo notebook a zadáš WiFi heslo a SSID vé domácí sítě. </br>
Na zivyobraz.eu pak zadáš MAC adresu zařízení a microESP bude posílat teplotu, vlhkost, tlak nebo koncentraci CO2 na Živý Obraz - záleží na připojeném čidle, naměřené údaje pak můžeš využít jak je libo - třeba je zobrazit na ePaper displeji. Navíc kód může řídit až 3 další zařízení přes GPIO.</br>
</br>
Podporované senzory jsou:</br>
SHT40 - teplota a vlhkost</br>
BME280 - tlak, teplota a vlhkost</br>
SCD41 - CO2, teplota, vlhkost</br>
</br> 
## Původní projekt
* Základní informace najdete na webu projektu: https://zivyobraz.eu/
* Konkrétní informace ohledně zprovoznění jsou v dokumentaci na adrese: https://wiki.zivyobraz.eu/
