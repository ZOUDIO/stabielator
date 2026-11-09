<!--
@file verschillen-plan-van-aanpak.md
@brief Verschillen, ingevulde keuzes en aanvullingen ten opzichte van het eerdere plan.
@date 2026-11-09
@author ProtoIoT Wietse Houwers
@note Prototype software for evaluation use only.
-->

# Verschillen met het plan van aanpak

Vergelijking: **plan v0.1 van 6 oktober 2026** met **firmware 0.5.1**, op 9 november 2026. De basiswerking blijft gelijk. Veel verschillen zijn invullingen van open keuzes uit het plan. Paginanummers verwijzen naar dat plan.

## Wat is anders of verder ingevuld?

| Onderwerp in het plan | Huidige uitvoering | Waarom / doel van de keuze |
| --- | --- | --- |
| Automatisch regelen na kalibratie (p. 2, 4). | Eerste gebruik gepauzeerd; daarna geldt de opgeslagen pauzekeuze, ook na herstart. | Voorkomt onverwacht hervatten na een bewuste stop. |
| Optionele automatische richtingskalibratie (p. 4, 6). | Handmatig instelbare motorindeling en asrotatie; geen automatische leerprocedure. | Aanpasbare montage zonder extra begeleide kalibratiestap. |
| Opstartkalibratie en wachttijd nog open (p. 8). | Iedere opstart en sensorreset opnieuw; standaard 3 seconden wachten en minimaal 1 seconde stil meten. | Een actuele referentie; bewegen tijdens kalibratie begint het meetvenster opnieuw. |
| Diagonale kanteling nog open (p. 8). | Standaard twee motoren; sterkste richting ook instelbaar. | Beide richtingen voelbaar maken en alternatieven kunnen testen. |
| Stopvoorwaarde nog open (p. 8). | Uitschakelhoek én maximaal 3 seconden per richting. Daarna eerst terug onder de grens. | Beperkt langdurig trillen en direct opnieuw activeren. |
| Vaste gebruiksduur of inactiviteit (p. 2, 8). | Standaard uit na 60 minuten zonder minstens 1° houdingsverandering; instelbaar. | Bij beweging kan het gebruik doorgaan. |
| Standaardwaarden en bereiken nog open (p. 8). | 1,5° aan, 1,0° uit, ongeveer 50% aansturing, 200/200 ms pulsen. Bereiken staan in de GUI. | Concreet startprofiel voor praktijktests; nog geen gevalideerde gebruiksinstelling. |
| WiFi-duur en heropenen nog open (p. 5, 8). | Netwerk standaard beschikbaar; opgeslagen aan/uit-keuze, herstel via BOOT 5 seconden. Routerverbinding optioneel. | Bereikbaar blijven zonder configuratietimer; AP uit bespaart energie. |
| Ledfuncties nog open (p. 8). | Kleuren voor normaal, pauze, accu, motoractiviteit, opstart, kalibratie en fouten; zie gebruiksaanwijzing. | Belangrijkste toestanden direct herkennen. |
| USB-debug en teststand voorzien (p. 6, 7). | Aparte keuzes behouden; live 3D toegevoegd. Handmatige tests kunnen kort trillen tijdens pauze. | Houding bekijken zonder automatische feedback te pauzeren; testpuls verandert de pauzekeuze niet. |
| BNO086 genoemd (p. 3); geen tweede motorprofiel beschreven. | BNO08x-bibliotheek; standaard ERM en aparte LRA-build. | Ondersteunt de sensorfamilie en verschillende motoren. Exact sensortype fysiek bevestigen. |
| Browserupdates niet beschreven. | Update via eigen instelnetwerk, met bestands- en variantcontrole. | Bijwerken zonder telkens USB te gebruiken. |

## Wat staat nog open?

Het plan vraagt complete praktijktests (p. 7). Builds en lokale software- en browsercontroles zijn uitgevoerd. Voelbaarheid, motorposities, montagehoudingen, accugrenzen en werking tijdens configureren moeten nog op de echte print worden gecontroleerd. Niet alle planeisen zijn dus al fysiek bewezen.
