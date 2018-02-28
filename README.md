# PapierKamienNozyce

Projekt Sieci Komputerowe 2. Maciej Rubczyński 106568. Jakub Gucenowicz 122470

Projekt to gra w papier, kamień, nożyce. Użytkownicy należą do drużyn i w ramach drużyn wybierają odpowiednie symbole.
Założenia projektu:
1.Gra zaczyna się w momencie startu serwera i toczy się na serwerze przez cały czas, wyświetlana jest informacja o zwycięstwach z rzedu drużyny.
2.Jeśli gracz dołączy do gry w trakcie, wybiera drużynę i może od razu grać.
3.Wybór drużyny jest permanentny dla nicku - logujac sie na serwer podaje sie nick i haslo (przechowywane jawnie), jesli uzytkownik juz jest w bazie programu, to gracz dołącza do gry, jesli nie - musi najpierw wybrac druzyne
4.Po zakończonej grze - jesli druzyna wygra, gracze ktorzy wybierali zwycięską opcję dostaja 1 punkt, jesli druzyna przegra i gracz wybral opcje, ktora przegrala - traci 1 punkt.
        Sila glosu na dane pole zalezy od punktow uzytkownika.
        Minimalna liczba punktów gracza wynosi 1.
5. W przypadku zerwania połączenia nic się nie dzieje, tzn. informacje o rozgrywce rozsylane są tylko do polaczanych i zalogowanych uzytkownikow
6. Jeśli wybór opcji nie jest jednoznaczny następuje wybór losowy.
        Oznacza to, że gra może toczyć się nawet wtedy, gdy nie ma żadnych graczy, bądź 1 drużyna może grać z komputerem.
7. Informacja o punktach zagłosowanych na dane pole wyboru jest pokazywana na żywo.

Projekt wykonywany w środowisku CodeBlocks.
