# Uređaj za dijeljenje karata
Uređaj koji automatski dijeli karte izrađen na ESP32 mikrokontroleru.
Uređaj dijeli špil od 32 karte četvorici igrača po pravilima igre belot (3 + 3 + 2) * 4, uz zvučnu najavu početka i kraja,
i prikaz stanja dijeljenja na OLED zaslonu.

## Funkcionalnost
Pritiskom tipke uređaj se pokreće te dijeljenje započinje: prije samog dijeljenja čuje se zvučni znak te istovremeno i na OLED
zaslonu piše kako dijeljenje započinje. Rotirajuća platforma, na kojoj su špil karata i DC motor, okreću se prema svakome igraču
te DC motor svakom od igrača izbacuje jednu po jednu kartu.

## Arhitektura sustava

### Blok dijagram sustava
                    ┌─────────────────┐
                    │  Vanjsko        │
                    │  napajanje      │
                    └────────┬────────┘
                             │ snaga
              ┌──────────────┼──────────────┐
              │              │              │
        ┌─────▼─────┐  ┌─────▼─────┐        │
        │  L9110    │  │  ULN2003  │        │
        │  H-most   │  │  driver   │        │
        └─────┬─────┘  └─────┬─────┘        │
              │              │              │
        ┌─────▼─────┐  ┌─────▼─────┐        │
        │ GA25-370  │  │ 28BYJ-48  │        │
        │ DC motor  │  │  koračni  │        │
        └───────────┘  └───────────┘        │
              ▲              ▲              │
              │ signal       │ signal       │
        ┌─────┴──────────────┴──────────────▼───┐
        │              ESP32                    │
        │  (upravljanje, bez prijenosa snage)   │
        └───┬──────────────┬──────────────┬─────┘
            │ I2C          │ UART         │ GPIO
      ┌─────▼─────┐  ┌─────▼─────┐  ┌─────▼─────┐
      │   OLED    │  │ DFPlayer  │  │   Tipka   │
      │  SSD1306  │  │   Mini    │  │           │
      └───────────┘  └─────┬─────┘  └───────────┘
                           │
                     ┌─────▼─────┐
                     │  Zvučnik  │
                     │  8Ω 0,5W  │
                     └───────────┘

### Mehanička konstrukcija
    ┌────────────────────────────────┐
    │   Spremnik špila + kotačić     │  ← okretna platforma
    └────────────────┬───────────────┘
                  ┌──┴──┐
                  │matica│
          ────────┼─────┼────────────
                  │vijak│
              ┌───┴─────┴───┐
              │ ležaj 625-ZZ│           ← nepomična baza
              └───┬─────┬───┘
                  │vijak│
                  └──┬──┘
              ┌──────┴──────┐
              │  spojnica   │
              └──────┬──────┘
              ┌──────┴──────┐
              │ 28BYJ-48    │
              └─────────────┘
                                        
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

## Programska izvedba

Za pokretanje i korištenje ESP32 koristio sam ESP-IDF v5.x bez grafičkog okruženja.
Uređivanje izvornog koda obavljeno je u Visual Studio Code-u, a prevođenje i prijenos
na mikrokontroler odrađeno je iz naredbenog retka.
Naredbe koje sam korsitio su : 
- idf.py set-target esp32 - ova naredba govori za koji čip se build-a, bira ispravan lanac alata i stvara sdkconfig datoteku s postavkama čipa
- idf.py build - prevodi izvorni kod u binarnu datoteku koju čip može izvršiti
- idf.py -p COM3 flash monitor - prijenos na ESP32 (flash-anje)
- 
### Struktura programa
Program je podijeljen na skupine funkcija ovisno o tome kojim uređajem se upravlja.
#### Upravljanje zaslonom
- oled_cmd(c) - šalje jednu naredbu upravljačkom sklopu
- oled_data(d, len) — šalje podatke u memoriju prikaza
- oled_init()  - izvodi inicijalizacijsku sekvencu
- oled_clear()  - briše cijeli prikaz
- oled_text(page, col, text) — ispisuje niz znakova na zadani položaj

#### Upravljanje reprodukcijom zvuka
- df_send_cmd(cmd, param) - sastavlja i šalje naredbeni okvir
- df_play_from_folder(folder, track)  - pokreće reprodukciju zadanog zapisa

#### Upravljanje step motorom
- stepper_off() - isključuje sve namotaje
- stepper_okreni(koraka, smjer)  - izvodi zadani broj koraka

#### Upravljanje DC motorom i logika dijeljenja
- izbaci_kartu() - jedan ciklus izbacivanja jedne karte
- podijeli_igracu(n) - izbacuje n karata jednom igraču
- runda(n)  - dijeli n karata svakom od četiri igrača

#### Očitavanje tipke
- cekaj_tipku() - blokira izvođenje do pritiska i otpuštanja
## Problemi s kojima sam se susreo

- ESP-IDF nije mogao izgraditi projekt na putanji s razmakom
- Neusklađenost brojeva pinova u kodu i na stvarnom spoju
- Nestabilnost napajanja pri pokretanju (Problem riješen zamijenom PowerBanka).
