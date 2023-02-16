# Sieci Komputerowe 2

#### *=== Gra Państwa-miasta ===*

## Autorzy:

- Patrycja Przybylska 146247
- Joanna Nuebauer 148111

## Use case:

*(wszytskie parametry ze znakiem * są konfigurowalnymi zmiennymi z pliku konfiguracyjnego)*

Gracz łączy się do serwera i wysyła swój nick (jeśli nick jest już zajęty,
serwer prosi o podanie innego nicku).

Gracz po wybraniu nicku trafia do lobby, w którym widzi czy aktualnie trwa rozgrywka (naraz odbywa się jedna rozgrywka z nieograniczoną liczbą graczy), czas pozostały do rozpoczęcia nowej rozgrywki (rozgrywka trwa maksymalnie pewien czas* plus bufor* pomiędzy rozgrywkami) oraz aktualną ilość graczy w lobby.

Jeżeli w aktualnie trwającej grze znajdzie się mniej niż 2 graczy, rozgrywka kończy się, server wyświetla finalny ranking z udziałem wszystkich graczy, którzy byli w grze od początku jej trwania. Pozostały gracz trafia do lobby.

Jeżeli w lobby znajdują się minimum 2 osoby to możliwe jest rozpoczęcie gry o ile aktualnie nie trwa inna gra. W tym momencie włącza się timer, który odlicza określony czas* po czym rozpoczyna rozgrywkę.

Po rozpoczęciu gry cyklicznie losowane są przez serwer kolejne litery alfabetu. Gracze mają określony czas* na wypełnienie rubryk tabelii.

Runda(składanie przez graczy odpowiedzi dla jedenj wylosowanej litery) kończy się po upłynięciu określonego czasu*.

Po wypełnieniu pól system podlicza punkty. Za unikalną odpowiedź przyznawane jest 10 puntków. W przypadku takich samych odpowiedzi u więcej niż jednego gracza, system przydziela pierwszemu graczowi, który przesłał identyczną odpowiedź pełną liczbę punktów, a pozostałym graczom z tą samą odpowiedzią - połowę pukntów. W przypadku braku odpowiedzi, przyznawane jest 0 pukntów.

Gracz może w każdym momentcie opuścić grę, jednak pozostaje on widoczny w rankingu. Rozgrywka trwa do upłynięcia maksymalnej ilośći czasu* lub pozostania mniej niż dwóch graczy.

Po zakończeniu rozgrywki serwer podlicza punkty i publikowany jest finalny ranking. Gracze wracają do lobby.
