#include <iostream>
#include <vector>
#include <algorithm>

#include <sstream>
#include <exception>
#include <typeinfo>
#include <fstream>
#include <time.h>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>


#ifdef __WIN32__
#include <winsock2.h>
#include <windows.h>
#include <windows.h>
#include <string>
#else
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////

// telnet localhost 2266

bool debug = true; //CZY DODATKOWE INFO
time_t dlugoscRundy = 3; //DLUGOSC RUNDY W SEKUNDACH
char ipSerwera [INET_ADDRSTRLEN] = "127.0.0.1"; //ADRES IP SERWERA
int portSerwera = 2266;
int wielkoscKolejki = 20;
int rozmiarBufora = 1000;

int socketSerwer;
struct sockaddr_in adresSerwer;
time_t pozostalyCzas = -1;

pthread_t watekNasluchujacy;
pthread_mutex_t mutexPozostalyCzas;

///////////////////////////////////////////////////////////////////////////////////////////

struct Gracz{
	int id;
	string nick;
	string haslo;
	int numerDruzyny;
	int liczbaPunktow;
	time_t ostatniaAktywnosc = time(NULL);
	int coWybral = -1; // -1 - nic, 0 - kamien, 1 - papier, 2 - nozyce

	int socketGracza = -1;
	struct sockaddr_in adresGracza;
};

struct Polaczenie{
    int socketPolaczenia;
    struct sockaddr_in adresPolaczenia;
};

struct Glos{
    int wybor;
    int iloscGlosow;
};



vector <Gracz> listaGraczy;

///////////////////////////////////////////////////////////////////////////////////////////

void wyswietlGraczy(){
	cout << "\n\n-------------------------------------";
	cout << "\nID\tNICK\tHASLO\tZESPOL\tPUNKTY\tIP\tSOCKET\tOSTATNIA_AKTYWNOSC";
	cout << "\n-------------------------------------";


	for (auto x: listaGraczy){

		char adresString[INET_ADDRSTRLEN];
        inet_ntop( AF_INET, & x.adresGracza.sin_addr.s_addr, adresString, INET_ADDRSTRLEN);

		cout << "\n"
            << x.id << "\t"
            << x.nick << "\t"
            << x.haslo << "\t"
            << x.numerDruzyny << "\t"
            << x.liczbaPunktow << "\t"
            << adresString << "\t"
            << to_string(x.socketGracza) << "\t"
            << x.ostatniaAktywnosc;
	}

	cout << "\n-------------------------------------\n";
}


void wyswietlInformacje(string informacja, bool pokazDate = true){

	if (pokazDate){

		char s[1000];

		time_t t = time(NULL);
		struct tm * p = localtime(&t);

		strftime(s, 1000, "%d.%m.%Y %H:%M:%S", p);

		cout << "<" << s << ">\t";
	}

	cout << informacja;

	cout << "\n";
}

///////////////////////////////////////////////////////////////////////////////////////////

int uzyskajPozycjeGraczaNaLiscie(int id){ // zwraca pozycje gracza; -1 gdy nie znaleziono

    for (int i= (int) listaGraczy.size()-1; i>=0; i--){
        if (listaGraczy.at(i).id == id){
            return i;
        }
    }

    wyswietlInformacje("ERROR: Nie znaleziono gracza na liscie, id gracza: " + to_string(id) );
    return -1;
}

int dodajGraczaDoListyGraczy( int id, string nick, string haslo, int numerDruzyny, int liczbaPunktow){ // zwraca : 0 - ok, -1 error

	bool czyGraczJestNaLiscie = false;

	try{

		unsigned int indeks;

		for (indeks=0; indeks < listaGraczy.size(); indeks++){ //sprawdzenie czy gracz juz jest na liscie
			if (listaGraczy.at(indeks).id == id){
				czyGraczJestNaLiscie = true;
				break;
			}
		}

		if (czyGraczJestNaLiscie){
			listaGraczy.at(indeks).ostatniaAktywnosc = time(NULL);
		}

		else{
			listaGraczy.push_back (Gracz());
			indeks = listaGraczy.size() -1;
			listaGraczy.at(indeks).id = id;
			listaGraczy.at(indeks).nick = nick;
			listaGraczy.at(indeks).haslo = haslo;
			listaGraczy.at(indeks).numerDruzyny = numerDruzyny;
			listaGraczy.at(indeks).liczbaPunktow = liczbaPunktow;
		}

	} catch (exception& e){
		cerr <<"\n\nERROR: listaGraczy: \n" << e.what() << "\n";
		return -1;
	}

	return 0;
}

int odczytajConfig( string sciezka ){ // zwraca : 0

	fstream config;
	config.open( sciezka );

	if (config.good()){
		string linia;
		char znak = '=';

		string zmienna, wartosc;

		while ( !config.eof() ){

			getline ( config, zmienna, znak );
			getline ( config, wartosc, '\n' );


			if (zmienna == "DEBUG"){
				if (wartosc == "true"){
					debug = true;
				}
				else{
					debug = false;
				}
			}
			else if (zmienna == "DLUGOSC RUNDY"){
				dlugoscRundy = atoi (wartosc.c_str() );
			}
			else if (zmienna == "IP SERWERA"){
				strcpy(ipSerwera, wartosc.c_str() );
			}
			else if (zmienna == "WIELKOSC KOLEJKI"){
                wielkoscKolejki = atoi (wartosc.c_str() );
			}
			else if (zmienna == "ROZMIAR BUFORA"){
                rozmiarBufora = atoi (wartosc.c_str() );
			}
		}
	}

	return 0;
}

int wczytajGracza(string sciezka, string nickWpisany, string hasloWpisany){ // zwraca : wartosc funkcji dodajGraczaDoListy; -1 error

	fstream bazaGraczy;
	bazaGraczy.open( sciezka );

		if ( bazaGraczy.good() ){

		char znak = ';';

		string id, numerDruzyny, liczbaPunktow;
		string nick, haslo;

		while ( !bazaGraczy.eof() ){

			getline ( bazaGraczy, id, znak );
			getline ( bazaGraczy, nick, znak );
			getline ( bazaGraczy, haslo, znak );
			getline ( bazaGraczy, numerDruzyny, znak );
			getline ( bazaGraczy, liczbaPunktow, znak );

			if (nick == nickWpisany && haslo == hasloWpisany){

				return dodajGraczaDoListyGraczy(
					atoi( id.c_str() ),
					nick,
					haslo,
					atoi ( numerDruzyny.c_str() ),
					atoi ( liczbaPunktow.c_str() )
				);

			}

		}

		bazaGraczy.close();
	}
	else{
		wyswietlInformacje("ERROR: " + sciezka + " nie istnieje.");
	}

	return -1;
}

int zamknijPolaczenieGracza( int id ){ // zwraca : 0 - ok, -1 error

    int i = uzyskajPozycjeGraczaNaLiscie(id);
    if (i > -1){
        if (listaGraczy.at(i).socketGracza > -1){
            if (close(listaGraczy.at(i).socketGracza) < 0){
                wyswietlInformacje("ERROR: close, id gracza: " + to_string(id) + ", kod bledu: " + to_string(errno) );
                return -1;
            }
            else{
                wyswietlInformacje("OK: close, id gracza: " + to_string(id) + ", socket: " + to_string(listaGraczy.at(i).socketGracza) );
                return 0;
            }
        }
    }

    return -1;

}

int wyslijWiadomosc(int nSocket, string wiadomoscString){ // zwraca : wartosc funkcji send (wyslaneBity) -> -1 error

        char wiadomosc[rozmiarBufora];

        for (int j=0; j < rozmiarBufora; j++){ //czyszczenie
            wiadomosc[j] = ' ';
        }

        strcpy(wiadomosc, wiadomoscString.c_str() );

        int dlugoscWiadomosci = sizeof (wiadomosc);
        int wyslaneBity;

        wyslaneBity = send(nSocket, wiadomosc, dlugoscWiadomosci, 0 );

        if ( wyslaneBity < rozmiarBufora){
            wyswietlInformacje("ERROR: send, socket: " + to_string(nSocket) + ", kod bledu: " + to_string(errno) );
        }
        else{
            wyswietlInformacje("Send, socket: " + to_string(nSocket) + ", bity wyslane: " + to_string(wyslaneBity) );
        }

        return wyslaneBity;

}

int wyslijWiadomoscDoGracza(int id, string wiadomoscString ){ // zwraca : wartosc funkcji wyslijWiadomosc, -1 error

    int i = uzyskajPozycjeGraczaNaLiscie(id); //pozycja

    if (i > -1){
        return wyslijWiadomosc(listaGraczy.at(i).socketGracza, wiadomoscString);
    }
    else{
        return -1;
    }


}

int wyslijDoWszystkich(string wiadomoscString){ // zwraca : 0
    for (int i= (int) listaGraczy.size()-1; i>=0; i--){
        if (listaGraczy.at(i).socketGracza > -1){

            wyslijWiadomoscDoGracza(listaGraczy.at(i).id, wiadomoscString);
        }
    }

    return 0;
}

int wyslijInformacjeOPartiiDoGracza(int id){ //zwraca wartosc funkcji wyslijWiadomoscDoGracza; -1 error

    string informacjaOPartii = "ObecnaPartia:pozostalyCzasSekundy=" + to_string(pozostalyCzas);

    int i = uzyskajPozycjeGraczaNaLiscie(id);

    if (i > -1){
        int druzynaGracza = listaGraczy.at(i).numerDruzyny;
        int wybory[3] = {0, 0, 0}; //wybory graczy

        for (auto x: listaGraczy){
            if (x.numerDruzyny == druzynaGracza){
                if ((x.coWybral >= 0) && (x.coWybral <= 2)){
                    wybory[x.coWybral] += x.liczbaPunktow;
                }
            }
        }

        informacjaOPartii += ";glosyKamien=" + to_string( wybory[0] )
                            += ";glosyPapier=" + to_string( wybory[1] )
                            += ";glosyNozyce=" + to_string( wybory[2] );
    }
    else{
        return -1;
    }


    return wyslijWiadomoscDoGracza(id, informacjaOPartii);
}

int wyslijInformacjeOPartiiDoWszystkich(){ //zwraca 0

    for (auto x: listaGraczy){
        wyslijInformacjeOPartiiDoGracza( x.id );
    }

    return 0;
}

int wyslijInformacjeONowejPartiiDoWszystkich(){ // zwraca wartosc funkcji wyslijDoWszystkich

    return wyslijDoWszystkich("NowaPartia:dlugoscRundySekundy=" + to_string(dlugoscRundy) );
}

int wyslijInformacjeOZakonczonejPartiiDoWszystkich(int zwycieskaDruzyna, int glosDruzyny0, int glosDruzyny1){


    return wyslijDoWszystkich("KoniecPartii:zwycieskaDruzyna=" + to_string(zwycieskaDruzyna)
                + ";glosDruzyny0=" + to_string(glosDruzyny0) + ";glosDruzyny1=" + to_string(glosDruzyny1));
}

int wyslijInformacjeOGraczuDoGracza(int id){ // zwraca wartosc funkcji wyslijWiadomoscDoGracza; -1 error

    int i = uzyskajPozycjeGraczaNaLiscie(id);

    if (i > -1){
        return wyslijWiadomoscDoGracza(id, "Gracz:nick=" + listaGraczy.at(i).nick
                + ";numerDruzyny=" + to_string(listaGraczy.at(i).numerDruzyny)
                + ";liczbaPunktow=" + to_string(listaGraczy.at(i).liczbaPunktow)
                + ";ostatniaAktywnosc=" + to_string(listaGraczy.at(i).ostatniaAktywnosc)
                + ";coWybral=" + to_string(listaGraczy.at(i).coWybral) );
    }
    else{
        return -1;
    }

}

int wyslijInformacjeOGraczuDoWszystkich(){ //zwraca 0

    for (auto x: listaGraczy){
        wyslijInformacjeOGraczuDoGracza(x.id);
    }

    return 0;
}

int rozlaczSocket (int nSocket){ //zwraca 0

    for (int i=0; i< (int) listaGraczy.size(); i++){
        if (listaGraczy.at(i).socketGracza == nSocket){
            listaGraczy.erase(listaGraczy.begin() + i);
            wyswietlInformacje("INFO: Usunieto gracza z listy; Socket: " + to_string(nSocket));
            break;
        }
    }

    close(nSocket);
    wyswietlInformacje("INFO: Socket rozlaczony: " + to_string(nSocket) );

    return 0;
}

string odbierzWiadomosc(int nSocket){ // zwraca : wiadomosc z komunikatu (string); pusta - error

        char wiadomosc[rozmiarBufora];
        string wiadomoscString = "";

        for (int j=0; j < rozmiarBufora; j++){ //czyszczenie bufora
            wiadomosc[j] = ' ';
        }

        int dlugoscWiadomosci = sizeof (wiadomosc);
        int otrzymaneBity;

        otrzymaneBity = recv(nSocket, wiadomosc, dlugoscWiadomosci, 0 );

        if ( otrzymaneBity == 0 ){
            wyswietlInformacje("ERROR: recv, socket: " + to_string(nSocket) + ", polaczenie przerwane!");
            rozlaczSocket(nSocket);
        }
        else if ( otrzymaneBity < rozmiarBufora){
            wyswietlInformacje("ERROR: recv, socket: " + to_string(nSocket) + ", otrzymane bity: " + to_string(otrzymaneBity) + ", kod bledu: " + to_string(errno) );
        }
        else{
            wyswietlInformacje("Recv, socket: " + to_string(nSocket) + ", bity otrzymane: " + to_string(otrzymaneBity) );

            int i=0;

            while ( ( i < rozmiarBufora) && (wiadomosc[i] != ' ') ){
                wiadomoscString += wiadomosc[i];
                i++;
            }

            wyswietlInformacje("Recv, socket: " + to_string(nSocket) + "; " + wiadomoscString);
        }

        return wiadomoscString;

}

int zaloguj (string nick, string haslo, int socketGracz, struct sockaddr_in adresGracz){ // zwraca : 0 - ok

    bool czySocketZostalDodany = false;
    int idGracza = -1;

	if (wczytajGracza("data/baza_graczy.txt", nick, haslo) == 0){

        for (int i= (int) listaGraczy.size()-1; i>=0; i--){

            if ( ( listaGraczy.at(i).nick == nick) && (listaGraczy.at(i).haslo == haslo) ){

                zamknijPolaczenieGracza( listaGraczy.at(i).id );
                listaGraczy.at(i).socketGracza = socketGracz;
                listaGraczy.at(i).adresGracza = adresGracz;
                idGracza = listaGraczy.at(i).id;
                czySocketZostalDodany = true;
                break;
            }
        }
    }

    if (czySocketZostalDodany){
		wyswietlInformacje("Pomyslnie zalogowano gracza: " + nick);

		if (debug){
			wyswietlGraczy();
		}

		wyslijWiadomoscDoGracza( idGracza, "Zaloguj:info=ok" );
		wyslijInformacjeOPartiiDoGracza( idGracza );
		wyslijInformacjeOGraczuDoGracza( idGracza );
	}
	else{

		wyswietlInformacje("Bledne logowanie gracza: " + nick);

		wyslijWiadomosc(socketGracz, "Zaloguj:info=error");
	}

	return 0;
}

void przyznajPunkty(int zwycieskaDruzyna, int zwycieskiGlos, int przegranyGlos){

    for (int i=0; i< (int) listaGraczy.size(); i++){

        if ((listaGraczy.at(i).numerDruzyny == zwycieskaDruzyna) && (listaGraczy.at(i).coWybral == zwycieskiGlos)){
            listaGraczy.at(i).liczbaPunktow += 1;
        }
        else if ((listaGraczy.at(i).numerDruzyny != zwycieskaDruzyna) && (listaGraczy.at(i).coWybral == przegranyGlos) && (listaGraczy.at(i).liczbaPunktow > 1) ){
            listaGraczy.at(i).liczbaPunktow -= 1;
        }
    }

}

int przypiszWyborDoGracza (int wybor, int nSocket){ //return 0 - ok; -1 error

    if ( (wybor >= -1) && (wybor <= 2) ){

        for (int i=0; i < (int) listaGraczy.size(); i++){
            if (listaGraczy.at(i).socketGracza == nSocket){

                listaGraczy.at(i).coWybral = wybor;
                wyslijInformacjeOPartiiDoWszystkich();
                return 0;
            }
        }

    }
    else{
        wyswietlInformacje ("ERROR: Niepoprawny wybor.");
        return -1;
    }

    return -1;

}

int przerobWiadomoscNaFunkcje( string wiadomoscString, int nSocket,  sockaddr_in nAdres ){ //return wartosc funkcji; -1 error

    if ( wiadomoscString != "" ){
        int rozmiarWiadomosci = (int) wiadomoscString.size();
        char wiadomosc[rozmiarWiadomosci];

        for (int i=0; i < rozmiarWiadomosci; i++){
            wiadomosc[i] = ' ';
        }

        strcpy(wiadomosc, wiadomoscString.c_str() );

        int i = 0;
        int j = 0;

        string funkcja = "";
        string zmienna;
        string wartosc;

        vector <string> zmienne;
        vector <string> wartosci;

        while ( (i < rozmiarWiadomosci ) && (wiadomosc[i] != ' ') && (wiadomosc[i] != ':') ){ //przypisanie do funkcji
            funkcja += wiadomosc[i];
            i++;
        }

        i++;


        while ( (i < rozmiarWiadomosci) && (wiadomosc[i] != ' ') ){

            zmienna = "";
            wartosc = "";

            while ( (i < rozmiarWiadomosci) && (wiadomosc[i] != ' ') && (wiadomosc[i] != '=') ){
                zmienna += wiadomosc[i];
                i++;
            }

            i++;

            while ( (i < rozmiarWiadomosci) && (wiadomosc[i] != ' ') && (wiadomosc[i] != ';') ){
                wartosc += wiadomosc[i];
                i++;
            }

            i++;

            if ( (zmienna != "") && (wartosc != "")){

                zmienne.push_back( zmienna );
                wartosci.push_back( wartosc );
                j++;
            }
            else{
                wyswietlInformacje("ERROR: Komunikat jest uszkodzony!");
                return -1;
            }

        }

        wyswietlInformacje("Funkcja: " + funkcja);

        for (int k=0; k < (int) zmienne.size(); k++){

            if (k <= (int) wartosci.size()){
                wyswietlInformacje(zmienne.at(k) + "=" + wartosci.at(k) + ";");
            }
        }

        if (funkcja == "Zaloguj"){
            if ( (zmienne.size() == wartosci.size() ) && ((int)zmienne.size() == 2) && (zmienne.at(0) == "nick") && (zmienne.at(1) == "haslo") ){
                return zaloguj( wartosci.at(0), wartosci.at(1), nSocket, nAdres);
            }
            else{
                wyswietlInformacje("ERROR: Komunikat jest uszkodzony!");
                return -1;
            }
        }
        else if (funkcja == "ObecnaPartia"){
             if ( (zmienne.size() == wartosci.size() ) && ((int)zmienne.size() == 1) && (zmienne.at(0) == "wybor") ){
                return przypiszWyborDoGracza( atoi( wartosci.at(0).c_str() ), nSocket);
            }
            else{
                wyswietlInformacje("ERROR: Komunikat jest uszkodzony!");
                return -1;
            }
        }
        else{
            wyswietlInformacje("ERROR: Funkcja nie istnieje!");
            return -1;
        }

    }
    else{
        wyswietlInformacje("ERROR: Funkcja jest niepoprawna!");
        return -1;
    }

}


void* funkcjaOdbierajacaWiadomosci(void *arguments){

    struct Polaczenie nowePolaczenie = *( (struct Polaczenie*) arguments );
    int socketPolaczenia = nowePolaczenie.socketPolaczenia;
    struct sockaddr_in adresPolaczenia = nowePolaczenie.adresPolaczenia;

    int poprawnosc = 1;

    //TODO!!
    while (poprawnosc > -1){
            wyswietlInformacje("INFO: Watek dziala.");

            string wiadomoscOdebrana = odbierzWiadomosc(socketPolaczenia);
            poprawnosc = przerobWiadomoscNaFunkcje(wiadomoscOdebrana, socketPolaczenia, adresPolaczenia);
    }

    rozlaczSocket( socketPolaczenia );
    wyswietlInformacje("INFO: Watek zatrzymany.");
}

void* funkcjaNasluchujacaPolaczen(void *arg){

    int sck = *((int*) arg);

    int socketPolaczenia;
    socklen_t rozmiarTmp;
    struct sockaddr_in adresPolaczenia;

    while (true){

        rozmiarTmp = sizeof( struct sockaddr_in );
        socketPolaczenia = accept( sck, (struct sockaddr*) &adresPolaczenia, &rozmiarTmp);

        if (socketPolaczenia < 1){
            wyswietlInformacje("ERROR: accept, kod bledu: " + to_string(errno) );
            //sleep(1);
        }
        else{
            //TODO - obsluga
            Polaczenie nowePolaczenie = Polaczenie();
            nowePolaczenie.socketPolaczenia = socketPolaczenia;
            nowePolaczenie.adresPolaczenia = adresPolaczenia;

            pthread_t watekOdczytujacy;

            int nWatekOdczytujacy = pthread_create( &watekOdczytujacy, NULL, &funkcjaOdbierajacaWiadomosci, &nowePolaczenie );

            if (nWatekOdczytujacy < 0){
                wyswietlInformacje("ERROR: Utworzenie watku, kod bledu: " + to_string(errno));
                //TODO - zamkniecie watku
            }


            //przerobWiadomoscNaFunkcje("Zaloguj:nick=user;haslo=password", socketPolaczenia, adresPolaczenia);
            //przerobWiadomoscNaFunkcje("ObecnaPartia:wybor=0", socketPolaczenia, adresPolaczenia);
        }

    }

    close(sck);

    pthread_exit(NULL);
}


int inicjujSerwer(){ // zwraca : 0 - ok, -1 error

	odczytajConfig("config.txt");

	srand( time( NULL ) );

	int nFoo = 1;
	int nBind;
	int nListen;
	int nWatekAkceptujacy;

	memset (&adresSerwer, 0, sizeof(struct sockaddr));
    adresSerwer.sin_family = AF_INET;
    adresSerwer.sin_addr.s_addr = inet_addr( (char*) &ipSerwera );
    adresSerwer.sin_port = htons( portSerwera );

	socketSerwer = socket(AF_INET, SOCK_STREAM, 0); // 0 - domyslny protokol

	if ( socketSerwer < 0 ){
		wyswietlInformacje("ERROR: Tworzenie - socket, kod bledu: " + to_string(errno));
		return -1;
	}

    setsockopt(socketSerwer, SOL_SOCKET, SO_REUSEADDR, (char*)&nFoo, sizeof(nFoo)); //rozwiazanie dla komunikatu błędu "Address already in use"

    nBind = bind(socketSerwer, (struct sockaddr*)&adresSerwer, sizeof(struct sockaddr));

    if (nBind < 0){
       wyswietlInformacje("ERROR: Tworzenie - bind, kod bledu: " + to_string(errno));
       return -1;
   }

    nListen = listen( socketSerwer, wielkoscKolejki );

    if (nListen < 0){
        wyswietlInformacje("ERROR: listen, kod bledu: " + to_string(errno));
        return -1;
    }

    nWatekAkceptujacy = pthread_create( &watekNasluchujacy, NULL, &funkcjaNasluchujacaPolaczen, &socketSerwer );

    if (nWatekAkceptujacy < 0){
        wyswietlInformacje("ERROR: Utworzenie watku, kod bledu: " + to_string(errno));
        return -1;
    }


	return 0;
}


void inicjujPartie(){

	wyswietlInformacje("Inicjowanie nowej partii...");

	for (int k = 0; k < (int) listaGraczy.size(); k++){
        listaGraczy.at(k).coWybral = -1;
	}

	wyslijInformacjeONowejPartiiDoWszystkich();
}

int petlaGry(){
	time_t start = time(NULL);
	time_t koniec = start + dlugoscRundy;

	wyswietlInformacje("---------Gra---------");

	while (time(NULL) < koniec){ // gra trwa okreslona liczbe sekund

        pthread_mutex_lock(&mutexPozostalyCzas);
        pozostalyCzas = koniec - time(NULL);
        pthread_mutex_unlock(&mutexPozostalyCzas);

        wyslijInformacjeOPartiiDoWszystkich();
		sleep(1); //test - uspij watek na 1 s
	}

    return 0;
}

void zakonczPartie(){ //podliczenie glosow, okreslenie zwycieskiego przedmotu, wybranie zwyciezcy. przyznanie punktow

	int iloscGlosow[2][3]; //2 druzyny, 3 wybory
	int max[2];
	int zwycieskiGlos[2];
	int zwycieskaDruzyna = -1;

	vector <Glos> zwycieskieGlosy_0;
	vector <Glos> zwycieskieGlosy_1;

	for (int i=0; i<2; i++){ //przypisanie zer
		max[i] = 0;

		for (int j=0; j<3; j++){
			iloscGlosow[i][j] = 0;
		}
	}

	for (auto x: listaGraczy){ //podliczenie glosow
		if (x.coWybral != -1){
			iloscGlosow[x.numerDruzyny][x.coWybral] += x.liczbaPunktow;
		}
	}

	for (int i=0; i < 2; i++){ //znalezienie max
		for (int j=0; j < 3; j++){
			if (iloscGlosow[i][j] > max[i]){
				max[i] = iloscGlosow[i][j];
			}
		}
	}

	for (int i=0; i < 3; i++ ){ //znalezienie zwycieskich glosow druzyn
		if ( max[0] == iloscGlosow[0][i] ){
			zwycieskieGlosy_0.push_back(Glos() );
            int index =  zwycieskieGlosy_0.size() -1;
            zwycieskieGlosy_0.at(index).wybor = i;
            zwycieskieGlosy_0.at(index).iloscGlosow = iloscGlosow[0][i];
		}
	}

	for (int i=0; i < 3; i++ ){ //znalezienie zwycieskich glosow druzyn
		if ( max[1] == iloscGlosow[1][i] ){
			zwycieskieGlosy_1.push_back(Glos() );
            int index =  zwycieskieGlosy_1.size() -1;
            zwycieskieGlosy_1.at(index).wybor = i;
            zwycieskieGlosy_1.at(index).iloscGlosow = iloscGlosow[1][i];
		}
	}

	wyswietlInformacje("Zwycieskie glosy <punkty> druzyny 0: ");

    for (auto x: zwycieskieGlosy_0){
        wyswietlInformacje(to_string(x.wybor) + ": <" + to_string(x.iloscGlosow) + ">" );
	}

	wyswietlInformacje("Zwycieskie glosy <punkty> druzyny 1: ");

    for (auto x: zwycieskieGlosy_1){
        wyswietlInformacje( to_string(x.wybor) + ": <" + to_string(x.iloscGlosow) + ">" );
	}

	int wyborDruzyny0 = rand() % zwycieskieGlosy_0.size();
	int wyborDruzyny1 = rand() % zwycieskieGlosy_1.size();

	zwycieskiGlos[0] = zwycieskieGlosy_0.at( wyborDruzyny0 ).wybor;
	zwycieskiGlos[1] = zwycieskieGlosy_1.at( wyborDruzyny1 ).wybor;


	wyswietlInformacje("---Druzyna 0---Max punktow: " + to_string(max[0]) );
	wyswietlInformacje("Zwycieski glos: " + to_string(zwycieskiGlos[0]) );
	wyswietlInformacje( "---Druzyna 1---Max punktow: " + to_string(max[1]) );
	wyswietlInformacje("Zwycieski glos: " + to_string(zwycieskiGlos[1]) );

	if (zwycieskiGlos[0] == zwycieskiGlos[1]){ //remis
		wyswietlInformacje("REMIS.");
	}
	else if (zwycieskiGlos[0] == 0 && zwycieskiGlos[1] == 1){ //kamien - papier
		zwycieskaDruzyna = 1;
	}
	else if (zwycieskiGlos[0] == 0 && zwycieskiGlos[1] == 2){ //kamien - nozyce
		zwycieskaDruzyna = 0;
	}
	else if (zwycieskiGlos[0] == 1 && zwycieskiGlos[1] == 0){ //papier - kamien
		zwycieskaDruzyna = 0;
	}
	else if (zwycieskiGlos[0] == 1 && zwycieskiGlos[1] == 2){ //papier - nozyce
		zwycieskaDruzyna = 1;
	}
	else if (zwycieskiGlos[0] == 2 && zwycieskiGlos[1] == 0){ //nozyce - kamien
		zwycieskaDruzyna = 1;
	}
	else if (zwycieskiGlos[0] == 2 && zwycieskiGlos[1] == 1){ //nozyce - papier
		zwycieskaDruzyna = 0;
	}

	if (zwycieskaDruzyna != -1){
		wyswietlInformacje("Zwyciezyla druzyna: " + to_string( zwycieskaDruzyna ) );
	}

	wyswietlInformacje("Przyznawanie punktow..");

    if ( (zwycieskaDruzyna == 0) || (zwycieskaDruzyna == 1) ){
        przyznajPunkty( zwycieskaDruzyna, zwycieskiGlos[zwycieskaDruzyna], zwycieskiGlos[abs(1-zwycieskaDruzyna)] );
    }

    wyslijInformacjeOZakonczonejPartiiDoWszystkich(zwycieskaDruzyna, zwycieskiGlos[0], zwycieskiGlos[1]);

    wyslijInformacjeOGraczuDoWszystkich();
}

void rozegrajPartie(){

	inicjujPartie();

	petlaGry();

	zakonczPartie();
}

int main(){

	wyswietlInformacje("Serwer - gra w kamien/papier/nozyce.");
	wyswietlInformacje("Ladowanie danych...");

	if ( inicjujSerwer() == 0){ //kod dla pomyslnego uruchomienia serwera - petla gry

        while (true){
            rozegrajPartie();
        }


	}
	else {
		wyswietlInformacje("Serwer nie moze zostac uruchomiony.");
	}

	return 0;
}
