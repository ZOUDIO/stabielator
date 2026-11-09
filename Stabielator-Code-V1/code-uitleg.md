<!--
@file code-uitleg.md
@brief Beknopt overzicht van de werking en belangrijkste bronbestanden.
@date 2026-11-09
@author ProtoIoT Wietse Houwers
@note Prototype software for evaluation use only.
-->

# Uitleg over de code

> Prototype software for evaluation use only.

De software volgt steeds dezelfde stappen: **houding meten → vergelijken met de referentie → passende motoren aansturen**. De webpagina draait ernaast. Daardoor blijft de regeling werken terwijl je de status bekijkt of instellingen aanpast.

## Waar staat wat?

Alle onderstaande bestanden staan in `src`.

| Bestand of groep | Taak |
| --- | --- |
| [main.cpp](../src/main.cpp) | Start het apparaat en verwerkt de algemene bediening. |
| [config.h](../src/config.h) | Bevat hardwareaansluitingen, firmwareversie en motorprofielen. |
| [imu_sensor.cpp](../src/imu_sensor.cpp), [stabilizer_logic.h](../src/stabilizer_logic.h) | Lezen de bewegingssensor en berekenen referentie, kanteling en feedback. |
| [stabilizer_control.cpp](../src/stabilizer_control.cpp) | Laat meting, motoren, accubewaking en opgeslagen instellingen samenwerken. |
| [haptic_control.cpp](../src/haptic_control.cpp), [haptic_pattern.h](../src/haptic_pattern.h) | Sturen de vier motoren aan en bepalen het pulspatroon. |
| [web_ui.h](../src/web_ui.h), [web_status_server.cpp](../src/web_status_server.cpp) | Bevatten de webpagina en verwerken opdrachten vanuit de browser. |
| [wifi_portal.cpp](../src/wifi_portal.cpp), [orientation_viewer.h](../src/orientation_viewer.h) | Verzorgen WiFi en de live 3D-weergave. |
| [power_monitor.h](../src/power_monitor.h), [led_status.h](../src/led_status.h), [firmware_update.cpp](../src/firmware_update.cpp) | Bewaken de accu, bepalen de ledkleur en verwerken firmware-updates. |

## Belangrijke afspraken

De referentie wordt bij iedere opstart opnieuw bepaald. Instellingen, pauzekeuze en teststand blijven wel bewaard. Oude of ongeldige sensormetingen en motorfouten blokkeren verdere feedback. Bij kritieke accuspanning of te lange stilstand probeert de software eerst de motoren te stoppen en schakelt daarna de voeding uit.

De gewone firmware gebruikt ERM-motoren. Voor LRA-motoren bestaat een aparte variant. Kies bij bouwen en bijwerken altijd het profiel dat bij de hardware past.

## Overige mappen

- `platformio.ini`: instellingen voor de ERM- en LRA-build.
- `scripts/package_ota.py`: maakt het updatebestand voor de webinterface.
- `dist/ota`: de gebouwde updatebestanden.
- `test`: controles van de regellogica en de GUI. De browsertests gebruiken een echt apparaat; motorbediening en uploaden worden alleen met een expliciete testoptie uitgevoerd.

De bestandscommentaren bevatten meer uitleg voor wie iets wil aanpassen. Een geslaagde softwarecontrole vervangt de praktijktest op de print niet.
