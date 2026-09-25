# Supla GUI-Generic z obsługą Serwomechanizmów 🛠️

Ten projekt to zmodyfikowana wersja potężnego oprogramowania **GUI-Generic** dla systemu Supla. Modyfikacja wprowadza nową warstwę sprzętową – w pełni konfigurowalną z poziomu przeglądarki natywną obsługę serwomechanizmów (180°, 360° i innych).

### 👑 Podziękowania dla Twórców GUI-Generic
Na wstępie chcę oddać ogromny szacunek i podziękować oryginalnym twórcom oraz kontrybutorom rdzenia GUI-Generic. Bez ich tytanicznej pracy, otwartego kodu i zaangażowania w społeczność Supli, ten moduł nigdy by nie powstał. Wielkie dzięki dla **krycha88**, **Espablo**, **Goral64**, **tomkowski78**, **lesny8** oraz całej ekipy zaangażowanej w ten projekt na GitHubie i na forum Supla.org!

### 🎯 Cel modyfikacji (Co chcę osiągnąć?)
Oryginalne GUI-Generic nie posiadało elastycznego wsparcia do zarządzania serwami prosto z interfejsu WWW. Mój cel to stworzenie kuloodpornego modułu sprzętowego dla płytek ESP8266 (np. Wemos D1 Mini) i ESP32, który pozwoli każdemu majsterkowiczowi podpiąć serwo i sterować nim bez dotykania linijki kodu. 

Chcę, aby system precyzyjnie kontrolował zawory wodne, rolety czy otwieracze okien na bazie kółka/suwaka w aplikacji Supla, z pełnym zabezpieczeniem mechanicznym przed uszkodzeniem zębatek.

### 🚧 Status Projektu (W budowie)
Projekt jest typu **Open Source** – bierzcie, testujcie, kopiujcie i róbcie z tym kodem, co tylko chcecie. Projekt aktualnie jest w fazie intensywnego rozwoju (Work In Progress). Chętnie przyjmę każdą poradę, konstruktywną krytykę i pomoc w łataniu ewentualnych błędów. 

**Na czym stanołem (Co już działa):**
* Zbudowany jest pełny interfejs WWW dla maksymalnie 5 niezależnych serw.
* Opanowałem konflikty skali protokołu Supli (prawidłowe przeliczanie zapytań `DIMMER` w skali 0-255 vs `ROLLERSHUTTER` w skali 0-100%).
* Zaimplementowałem "Księgę Logiki" – precyzyjną matematykę rozciągającą dowolny suwak z aplikacji na konkretny, zadeklarowany kąt serwa (np. suwak 0-100% precyzyjnie operujący w bezpiecznym, mechanicznym przedziale 45°).
* Wyeliminowałem zjawisko gubienia pamięci i gwałtownego ruszania serw przy restarcie zasilania mikrokontrolera (ochrona przed "brownoutem").
* Wdrożono omijanie tarcia statycznego (Deadband) oraz sprzętowe odpinanie pinu PWM po ruchu (co eliminuje denerwujące buczenie silników w spoczynku).

**Nad czym będę pracował dalej:**
* Klasyfikacja kolejnych przedziałów dla serw niestandardowych (np. 270 stopni).
* Dopracowanie asymetrycznych czasów jazdy w serwach 360° pracujących jako rolety.
* Doskonalenie procedury awaryjnego odcinania zasilania przy fizycznym zwarciu krańcówek.
* Dalsza walka z cenną pamięcią RAM na ESP8266 i optymalizacja samego C++.

### ☕ Słowo końcowe
Na ten moment wypycham ten kod do Was i muszę zrobić sobie chwilę przerwy. Po ostatnich ciężkich bojach z matematyką, skaczącymi napięciami, PWM-em i wybuchającą pamięcią na ESP mam po prostu dość na ten tydzień! 😁 

Korzystajcie mądrze i nie połamcie orczyków!

— **Autor Modułu Serwa: Kamil Ból "Bólu"**
