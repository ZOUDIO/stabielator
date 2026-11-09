<!--
@file gebruiksaanwijzing.md
@brief Eenvoudige bediening van de Stabielator en de webinterface.
@date 2026-11-09
@author ProtoIoT Wietse Houwers
@note Prototype software for evaluation use only.
-->

# Gebruiksaanwijzing

De Stabielator meet kanteling ten opzichte van een gekozen houding. Vier trilmotoren geven feedback over de richting.

## Aan de slag

1. Schakel het apparaat in en houd het stil in de gewenste houding.
2. Verbind met **STABIELATOR-…**, standaard zonder wachtwoord. Kies zo nodig voor verbonden blijven zonder internet.
3. Open **http://192.168.4.1/basis**.
4. Wacht op de referentiekalibratie: standaard 3 seconden wachten en minimaal 1 seconde stilhouden. **Kalibreren** kiest opnieuw een referentie.
5. Kies **Hervatten**. Zet bij Geavanceerd ook **Teststand uit** als die aan staat.

Bij eerste gebruik start het apparaat gepauzeerd. Daarna blijft de opgeslagen pauzekeuze gelden. Kalibratie gebeurt bij iedere opstart opnieuw.

## Dagelijkse bediening

- **Opslaan:** bewaart instellingen.
- **Stoppen en bewaren:** stopt feedback, ook na opnieuw inschakelen.
- **Hervatten:** geeft feedback weer vrij zodra sensor en motoren gereed zijn.
- **Test patroon:** test één motor maximaal 3 seconden met de opgeslagen instellingen. Werkt ook tijdens pauze; de pauzekeuze blijft bewaard.

De standaardinstellingen zijn:

| Instelling | Waarde |
| --- | --- |
| Feedback aan / uit | Vanaf 1,5° / bij 1,0° of minder |
| Trilintensiteit | Ongeveer 50% van de aansturing |
| Pulspatroon | 200 ms aan, 200 ms uit |
| Maximale feedback | 3 seconden per richting, inclusief uit-fasen |
| Automatisch uitschakelen | Na 60 minuten zonder voldoende beweging |

Na de maximale trilduur moet je eerst terug onder de uitschakelhoek voordat die richting opnieuw trilt. Het accupercentage is een schatting.

## Geavanceerd en de statusled

**Geavanceerd** bevat motorindeling en diagnose. **Teststand aan** pauzeert de automatische feedback. **USB-debug en live 3D** staat daar los van: kies Aan en sla op voor live houding.

| Led | Betekenis |
| --- | --- |
| Groen | Normale werking; korte blauwe flits bij verbonden WiFi-client |
| Rood | Pauze of lage accu; lees de melding op de pagina |
| Oranje | Motoruitgang actief |
| Blauw / cyaan knipperend | Opstarten / kalibreren |
| Magenta knipperend | Sensor- of motorfout |

Lage accu en hardwarefouten hebben voorrang op de pauzekleur. Tijdens opgeslagen pauze blijft de indicatie zichtbaar, ook bij led uit.

## Bijwerken en hulp

Kies bij **Geavanceerd > Firmware bijwerken** het passende **ERM- of LRA-**`.stab.bin`-bestand uit `dist/ota`. Houd voeding en instelnetwerk verbonden tot de update bevestigd is.

Instelnetwerk kwijt? Houd **BOOT 5 seconden** vast. Geen feedback? Controleer pauze, teststand, kalibratie en foutmeldingen. Motorstop mislukt? Schakel uit en controleer de hardware.
