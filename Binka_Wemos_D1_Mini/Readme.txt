Instrukcja wgrywania (Flashowanie)

Do modułu najlepiej wgrać plik `firmware.bin` programem **ESP Flash Download Tool** z następującymi ustawieniami:

* **SPI SPEED:** 40Mhz
* **SPI MODE:** QIO
* **FLASH SIZE:** 8 Mbit (czyli 1MByte)
* **BAUDRATE:** 115200

Wskazujesz plik binarnego softu, wpisujesz adres `0x00000`, klikasz **ERASE** (żeby wyczyścić śmieci), a potem **START**. Po restarcie Wemosa szukasz sieci Wi-Fi zaczynającej się od "SUPLA-..." i konfigurujesz.
