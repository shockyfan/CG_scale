#CG scale (modified)

Der Originale Code stammt von Nightflyer88 und ist hier zu finden:
https://github.com/nightflyer88/CG_scale

Da in letzter Zeit keine Entwicklung mehr stattgefunden hat und ich ein paar Ideen eingbringen möchte, habe ich mich für diesen Fork entschlossen.

Bisher umgesetzt:
- Restrukturierung für PlatformIO, was ein einfacheres Builden und Dependencies Management ermöglicht
- Update einiger Dependencies
- Fixes von Compilerwarnungen
- Implementierung von ESP Now zur Kommunikation mit dem Servotester Deluxe, dieser ist nun in der Lage als Kabelloses Display zu fungieren. (Mein Fork mit entsprechender Funktionalität und anderen Erweiterungen: https://github.com/shockyfan/Servotester_Deluxe)

Worauf beim Flashen zu Achten ist:
- Spiffs wurde durch LittleFS ersetzt, dadruch muss die UI neu aufgespielt werden
- Bitte vorher Kalibrierdaten übers WebInterface oder die Konsole notieren damit anschließend nichts neu kalibriert werden muss
- Aktuell nur mit dem ESP8266 getestet
- Zum Verwenden der kabellosen Display Funktion muss zuerst das ensprechende Menü im Servotester ausgewählt und dann die Waage eingeschaltet werden. Diese bootet dann im ESP Not Modus, das WebInterface ist währendessen nicht verfügbar
- Der Modus mit getrennten Waagen für 2 oder 3 Beinfahrwerke ist nicht getestet und wird aktuell nicht funktionieren, steht auf der ToDo.