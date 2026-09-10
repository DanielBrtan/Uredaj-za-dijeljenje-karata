# Uredaj-za-dijeljenje-karata
Uređaj koji automatski dijeli karte izrađen na ESP32 mikrokontroleru.
Uređaj dijeli špil od 32 karte četvorici igrača po pravilima igre belot (3 + 3 + 2) * 4, uz zvučnu najavu početka i kraja,
i prikaz stanja dijeljenja na OLED zaslonu.

#Funkcionalnost
Pritiskom tipke uređaj se pokreće te dijeljenje započinje: prije samog dijeljenja čuje se zvučni znak te istovremeno i na OLED
zaslonu piše kako dijeljenje započinje. Rotirajuća platforma, na kojoj su špil karata i DC motor, okreću se prema svakome igraču
te DC motor svakom od igrača izbacuje jednu po jednu kartu.

## Komponente
| Komponenta | Model | Uloga |
|---|---|---|
| Mikrokontroler | ESP32-WROOM-32 | upravljanje |
| DC motor | GA25-370 s enkoderom | izbacivanje karata |
| Driver DC motora | L9110 | H-most |
| Step motor | 28BYJ-48 | rotacija platforme |
| Driver step motora | ULN2003 | upravljanje namotajima |
| Zvuk | DFPlayer Mini + zvučnik 8Ω 0.5W | reprodukcija zvuka |
| Zaslon | SSD1306 0.96" 128x64 (I2C) | prikaz stanja |
| Tipka | tact switch 6x6 mm | pokretanje |

## Shema spajanja
| ESP32 | Spojeno na |
|---|---|
| GPIO25 | L9110 IA2 |
| GPIO33 | L9110 IB2 |
| GPIO27, 14, 12, 13 | ULN2003 IN1–IN4 |
| GPIO21 | DFPlayer RX (preko otpornika 1 kΩ) |
| GPIO22 | DFPlayer TX |
| GPIO18 | OLED SDA |
| GPIO19 | OLED SCL |
| GPIO32 | tipka (drugi kraj na GND) |
| GPIO16, 17 | enkoder DC motora |

Motori i ESP32 se napajaju preko vanjskog izvora (PowerBank).

## Pokretanje

Za pokretanje i korištenje ESP32 koristio sam ESP-IDF v5.x bez grafičkog okruženja.
Naredbe koje sam korsitio su : idf.py set-target esp32, idf.py build, idf.py -p COM3 flash monitor
                              
## Problemi s kojima sam se susreo

-ESP-IDF nije mogao izgraditi projekt na putanji s razmakom
-Neusklađenost brojeva pinova u kodu i na stvarnom spoju
-Nestabilnost napajanja pri pokretanju (Problem riješen zamijenom PowerBanka).
