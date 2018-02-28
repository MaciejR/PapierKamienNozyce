#include <gtk/gtk.h>
#include<stdio.h>
#include<stdlib.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string.h>
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
#include <X11/Xlib.h>

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
 GtkWidget *window;
 GtkWidget *image;
 GtkWidget *button;
 GtkWidget *box;
 GtkWidget *textCzas2;
 GtkWidget *textLiczbaPunktow2;
 GtkWidget *textGlosy1;
 GtkWidget *textGlosy2;
 GtkWidget *textGlosy0;
 GtkWidget *textDruzyna1;
 GtkWidget *textDruzyna2;
 GtkWidget *textNazwaDruzyny;
 GtkWidget *textTwojWybor;
 GtkWidget *textZwyciestwo;
 GtkWidget *textAktualnyWyborNapis;
 GtkWidget *frame;
 GtkWidget *fixed1;

 GtkWidget *textLogin;
 GtkWidget *textHaslo;
 GtkWidget *textLoginNazwa;
 GtkWidget *textHasloNazwa;
 GtkWidget *textBledneLogowanie;
 GtkWidget *zalogujPrzycisk;

 int mojaDruzyna;
 int mojWybor=-1;
bool debug = true; //CZY DODATKOWE INFO
time_t dlugoscRundy = 3; //DLUGOSC RUNDY W SEKUNDACH
char ipSerwera [INET_ADDRSTRLEN] = "127.0.0.1"; //ADRES IP SERWERA
int portSerwera = 2266;
int wielkoscKolejki = 20;
int rozmiarBufora = 1000;
bool czyWybral[2]={false,false};
int socketSerwer;
struct sockaddr_in adresSerwer;
time_t pozostalyCzas = -1;
const gchar *login;
const gchar *haslo;
bool czyZalogowany= false;

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



vector <Gracz> listaGraczy;


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
           close(socketSerwer);
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
        wyswietlInformacje(wiadomoscString);
        return wiadomoscString;

}

int inicjujKlienta(){ // zwraca : 0 - ok, -1 error

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
		wyswietlInformacje("ERROR: Tworzenie - socket, kod bledu: ");// + to_string(errno));
		return -1;
	}

    setsockopt(socketSerwer, SOL_SOCKET, SO_REUSEADDR, (char*)&nFoo, sizeof(nFoo)); //rozwiazanie dla komunikatu błędu "Address already in use"


    if (connect (socketSerwer, (struct sockaddr*) &adresSerwer, sizeof adresSerwer) < 0) {
		//perror ("Brak połączenia");
		//exit (EXIT_FAILURE);
		return -1;
	}else{
	printf ("Połączenie udane");
	}





	return 0;
}
 void wypiszSymbol ( int numer, GtkWidget* label ){ //Zmienia 0.1.2 na kamień/nozyce/papier
if(numer==0){
gtk_label_set_text ((GtkLabel*)label,"Kamień");
} else if(numer==1){
gtk_label_set_text ((GtkLabel*)label,"Papier");
} else  if(numer==2){
gtk_label_set_text ((GtkLabel*)label,"Nożyce");
}

}
 void akcjaPrzycisk ( GtkWidget *widget,
                        int numer)
{
wyslijWiadomosc(socketSerwer,"ObecnaPartia:wybor="+to_string(numer)+" ");
czyWybral[0]=true;
wypiszSymbol(numer,textAktualnyWyborNapis);
mojWybor=numer;
}

void inicjujMenuGUI(){



    gtk_fixed_put (GTK_FIXED (fixed1), textLogin, 550, 0);
    gtk_fixed_put (GTK_FIXED (fixed1), textHaslo, 550, 70);
    gtk_fixed_put (GTK_FIXED (fixed1), zalogujPrzycisk, 200, 210);
    gtk_widget_set_visible (zalogujPrzycisk,false);


    gtk_label_set_text(GTK_LABEL(textLoginNazwa),"Czas do końca rundy:");
    gtk_fixed_put (GTK_FIXED (fixed1), textLoginNazwa, 550, 0);
    gtk_widget_show (textLoginNazwa);



    gtk_fixed_put (GTK_FIXED (fixed1), textCzas2, 830, 0);
    gtk_widget_show (textCzas2);

     gtk_label_set_text(GTK_LABEL(textHasloNazwa),"Liczba Punktów:");
    gtk_fixed_put (GTK_FIXED (fixed1), textHasloNazwa, 550, 70);
    gtk_widget_show (textHasloNazwa);



    gtk_fixed_put (GTK_FIXED (fixed1), textLiczbaPunktow2, 830, 70);
    gtk_widget_show (textLiczbaPunktow2);

    GtkWidget *textOstatnia;
    textOstatnia=gtk_label_new ("Ostatnia rozgrywka");
    gtk_fixed_put (GTK_FIXED (fixed1), textOstatnia, 550, 140);
    gtk_widget_show (textOstatnia);



    gtk_label_set_text(GTK_LABEL(textZwyciestwo),"-");
    gtk_fixed_put (GTK_FIXED (fixed1), textZwyciestwo, 200, 140);
    gtk_widget_show (textZwyciestwo);

    gtk_label_set_text(GTK_LABEL(textBledneLogowanie),"Twój wybór:");
    gtk_fixed_put (GTK_FIXED (fixed1), textBledneLogowanie, 0, 310);
    gtk_widget_show (textBledneLogowanie);


    gtk_fixed_put (GTK_FIXED (fixed1), textAktualnyWyborNapis, 200, 310);
    gtk_widget_show (textAktualnyWyborNapis);


    textGlosy0=gtk_label_new ("0");
    gtk_fixed_put (GTK_FIXED (fixed1), textGlosy0, 60, 375);
    gtk_widget_show (textGlosy0);

    textGlosy1=gtk_label_new ("0");
    gtk_fixed_put (GTK_FIXED (fixed1), textGlosy1, 260, 375);
    gtk_widget_show (textGlosy1);

    textGlosy2=gtk_label_new ("0");
    gtk_fixed_put (GTK_FIXED (fixed1), textGlosy2, 450, 375);
    gtk_widget_show (textGlosy2);

    GtkWidget *textDruzyna1nazwa;
    textDruzyna1nazwa=gtk_label_new ("Drużyna nr 0:");
    gtk_fixed_put (GTK_FIXED (fixed1), textDruzyna1nazwa, 550,210);
    gtk_widget_show (textDruzyna1nazwa);

     textDruzyna1=gtk_label_new ("");
    gtk_fixed_put (GTK_FIXED (fixed1), textDruzyna1, 800,210);
    gtk_widget_show (textDruzyna1);

    GtkWidget *textDruzyna2nazwa;
    textDruzyna2nazwa=gtk_label_new ("Drużyna nr 1:");
    gtk_fixed_put (GTK_FIXED (fixed1), textDruzyna2nazwa, 550,280);
    gtk_widget_show (textDruzyna2nazwa);

     textDruzyna2=gtk_label_new ("");
    gtk_fixed_put (GTK_FIXED (fixed1), textDruzyna2, 800, 280);
    gtk_widget_show (textDruzyna2);

      GtkWidget *textTwojWyborNazwa;
    textTwojWyborNazwa=gtk_label_new ("Twój Wybór:");
    gtk_fixed_put (GTK_FIXED (fixed1), textTwojWyborNazwa, 550,350);
    gtk_widget_show (textTwojWyborNazwa);

    textTwojWybor=gtk_label_new ("");
    gtk_fixed_put (GTK_FIXED (fixed1), textTwojWybor, 800, 350);
    gtk_widget_show (textTwojWybor);


    GtkWidget *textNazwaDruzynyNazwa;
    textNazwaDruzynyNazwa=gtk_label_new ("Numer Druzyny:");
    gtk_fixed_put (GTK_FIXED (fixed1), textNazwaDruzynyNazwa, 0,0);
    gtk_widget_show (textNazwaDruzynyNazwa);

    textNazwaDruzyny=gtk_label_new ("");
    gtk_fixed_put (GTK_FIXED (fixed1), textNazwaDruzyny, 220, 0);
    gtk_widget_show (textNazwaDruzyny);


    button = gtk_button_new();

    g_signal_connect (button, "clicked",
		      G_CALLBACK (akcjaPrzycisk), (gpointer) 0);

    gtk_fixed_put (GTK_FIXED (fixed1), button, 0, 450);
    image = gtk_image_new_from_file ("0.png");
    gtk_button_set_image (GTK_BUTTON (button), image);

    gtk_widget_show (button);

    button = gtk_button_new();

    g_signal_connect (button, "clicked",
    G_CALLBACK (akcjaPrzycisk), (gpointer) 1);

    gtk_fixed_put (GTK_FIXED (fixed1), button, 200, 450);
    image = gtk_image_new_from_file ("1.png");
    gtk_button_set_image (GTK_BUTTON (button), image);

    gtk_widget_show (button);

    button = gtk_button_new();


    g_signal_connect (button, "clicked",
		      G_CALLBACK (akcjaPrzycisk), (gpointer) 2);

    gtk_fixed_put (GTK_FIXED (fixed1), button, 400, 450);
    image = gtk_image_new_from_file ("2.png");
    gtk_button_set_image (GTK_BUTTON (button), image);

    gtk_widget_show (button);







}
int przerobWiadomoscNaFunkcje( string wiadomoscString ){ //return wartosc funkcji; -1 error

    if ( wiadomoscString != "" ){
        int rozmiarWiadomosci = wiadomoscString.size();
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
                wyswietlInformacje("ERROR: Komunikat jest uszkodzony11!");
                return -1;
            }

        }

        wyswietlInformacje("Funkcja: " + funkcja);

        for (int k=0; k < zmienne.size(); k++){

            if (k <= wartosci.size()){
                wyswietlInformacje(zmienne.at(k) + "=" + wartosci.at(k) + ";");
            }
        }

        if (funkcja == "Zaloguj"){

            if ( (zmienne.size() == wartosci.size() ) && (zmienne.size() == 2) && (zmienne.at(0) == "nick") && (zmienne.at(1) == "haslo") ){
                wyslijWiadomosc(socketSerwer,"Zaloguj:nick="+string(login)+ ";haslo="+string(haslo)+"  ");
            }
            if ( (zmienne.size() == wartosci.size() ) && (zmienne.size() == 1) && (zmienne.at(0) == "info")){
                if(wartosci.at(0)[0]=='o'){
                czyZalogowany=true;
                inicjujMenuGUI();

                }else{
                gtk_label_set_text ((GtkLabel*)textBledneLogowanie,"Spróbuj Ponownie");


                }
            }
            else{
                wyswietlInformacje("ERROR: Komunikat jest uszkodzony1!");
                return -1;
            }
        }
        else if (funkcja == "ObecnaPartia"){
             if ( (zmienne.size() == wartosci.size() ) && (zmienne.size() == 4) && (zmienne.at(0) == "pozostalyCzasSekundy") ){
                //Wyświetlanie danych odnośnie obecnej partii
                gtk_label_set_text ((GtkLabel*)textCzas2,wartosci.at(0).c_str());
                gtk_label_set_text ((GtkLabel*)textGlosy0,wartosci.at(1).c_str());
                gtk_label_set_text ((GtkLabel*)textGlosy1,wartosci.at(2).c_str());
                gtk_label_set_text ((GtkLabel*)textGlosy2,wartosci.at(3).c_str());

            }
            else{
                wyswietlInformacje("ERROR: Komunikat jest uszkodzony!");
                return -1;
            }
        }
        else if(funkcja == "KoniecPartii"){
            if ( (zmienne.size() == wartosci.size() ) && (zmienne.size() == 3) && (zmienne.at(0) == "zwycieskaDruzyna") ){
                if(atoi( wartosci.at(0).c_str())==mojaDruzyna){
                       wyswietlInformacje(wartosci.at(mojaDruzyna+1));
                        wyswietlInformacje(to_string(mojWybor));
                    if(atoi( wartosci.at(mojaDruzyna+1).c_str())==mojWybor){

                        gtk_label_set_text ((GtkLabel*)textZwyciestwo,"ZWYCIĘSTWO: +1 PUNKT");
                    }else{
                        gtk_label_set_text ((GtkLabel*)textZwyciestwo,"ZWYCIĘSTWO");
                    }

                }else if((atoi( wartosci.at(0).c_str())==-1)){
                    gtk_label_set_text ((GtkLabel*)textZwyciestwo,"REMIS   ");
                }else {
                    gtk_label_set_text ((GtkLabel*)textZwyciestwo,"PORAŻKA");
                }
                wypiszSymbol(atoi( wartosci.at(1).c_str()),textDruzyna1);
                wypiszSymbol(atoi( wartosci.at(2).c_str()),textDruzyna2);
                czyWybral[1]=czyWybral[0];
                czyWybral[0]=false;
                gtk_label_set_text ((GtkLabel*)textAktualnyWyborNapis,"-");
                mojWybor=-1;
            }

            else{
                wyswietlInformacje("ERROR: Komunikat jest uszkodzony!");
                return -1;
            }
        }
        else if (funkcja == "NowaPartia"){
             if ( (zmienne.size() == wartosci.size() ) && (zmienne.size() == 1) && (zmienne.at(0) == "dlugoscRundySekundy") ){
                dlugoscRundy=(time_t)atoi(wartosci.at(0).c_str());


            }
            else{
                wyswietlInformacje("ERROR: Komunikat jest uszkodzony!");
                return -1;
            }

        }
        else if(funkcja == "Gracz"){
            if ( (zmienne.size() == wartosci.size() ) && (zmienne.size() == 5) && (zmienne.at(0) == "nick") ){

                gtk_label_set_text ((GtkLabel*)textLiczbaPunktow2,wartosci.at(2).c_str());
                if(czyWybral[1]==true)wypiszSymbol(atoi( wartosci.at(4).c_str()),textTwojWybor);
                else {gtk_label_set_text ((GtkLabel*)textTwojWybor,"-");}

                gtk_label_set_text ((GtkLabel*)textNazwaDruzyny,wartosci.at(1).c_str());
                mojaDruzyna=atoi(wartosci.at(1).c_str());


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

        return -1;
    }

}
void* funkcjaOdbierajacaWiadomosci(void *arguments){

    struct Polaczenie nowePolaczenie = *( (struct Polaczenie*) arguments );
    int socketPolaczenia = nowePolaczenie.socketPolaczenia;
    struct sockaddr_in adresPolaczenia = nowePolaczenie.adresPolaczenia;

    int poprawnosc = 1;


    while (poprawnosc > -1){
            wyswietlInformacje("INFO: Watek dziala.");

            string wiadomoscOdebrana = odbierzWiadomosc(socketPolaczenia);
            poprawnosc = przerobWiadomoscNaFunkcje(wiadomoscOdebrana);
    }
    if(czyZalogowany==true){
    gtk_widget_set_visible (textZwyciestwo,false);
    gtk_widget_set_visible (zalogujPrzycisk,true);
    gtk_label_set_text(GTK_LABEL(textBledneLogowanie),"Brak połączenia:");
    gtk_fixed_put (GTK_FIXED (fixed1), textBledneLogowanie, 200, 140);
    }

}



 void zaloguj ( GtkWidget *widget)
{

login=gtk_entry_get_text (GTK_ENTRY(textLogin) );

haslo=gtk_entry_get_text (GTK_ENTRY(textHaslo) );


if(inicjujKlienta()==-1){
gtk_label_set_text((GtkLabel*)textBledneLogowanie,"Brak połączenia!");
close(socketSerwer);
 }else{
Polaczenie nowePolaczenie = Polaczenie();
            nowePolaczenie.socketPolaczenia = socketSerwer;
            nowePolaczenie.adresPolaczenia = adresSerwer;

            pthread_t watekOdczytujacy;

            int nWatekOdczytujacy = pthread_create( &watekOdczytujacy, NULL, &funkcjaOdbierajacaWiadomosci, &nowePolaczenie );

            if (nWatekOdczytujacy < 0){
                wyswietlInformacje("ERROR: Utworzenie watku, kod bledu: " + to_string(errno));
}



if(przerobWiadomoscNaFunkcje("Zaloguj:nick="+string(login)+ ";haslo="+string(haslo)+" ")==-1){

gtk_label_set_text((GtkLabel*)textBledneLogowanie,"Spróbuj ponownie!");

}

}
}

void inicjujGUI(int argc,char* argv[]) {

gint i;
XInitThreads(); //Włączenie wątków dla GTK
  /* Initialise GTK */
  gtk_init (&argc, &argv);

  /* Create a new window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Papier Kamień Nożyce");

//styl Css dla GUI
    GtkCssProvider *cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(cssProvider, "theme.css", NULL);
      gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(cssProvider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);
g_signal_connect (window, "destroy",
		    G_CALLBACK (gtk_main_quit), NULL);

  /* Sets the border width of the window. */
  gtk_container_set_border_width (GTK_CONTAINER (window), 15);

  /* Create a Fixed Container */
  fixed1 = gtk_fixed_new ();
  gtk_container_add (GTK_CONTAINER (window), fixed1);
  gtk_widget_show (fixed1);
//do logowania TODO

textBledneLogowanie=gtk_label_new("Witaj w rozgrywce!");
                gtk_fixed_put (GTK_FIXED (fixed1), textBledneLogowanie, 70, 0);
                gtk_widget_show (textBledneLogowanie);
  textLoginNazwa=gtk_label_new("Login:");
   gtk_fixed_put (GTK_FIXED (fixed1), textLoginNazwa, 30, 80);
    gtk_widget_show (textLoginNazwa);

  textHasloNazwa=gtk_label_new("Hasło:");
   gtk_fixed_put (GTK_FIXED (fixed1), textHasloNazwa, 30, 150);
    gtk_widget_show (textHasloNazwa);

    textLogin=gtk_entry_new();
    gtk_fixed_put (GTK_FIXED (fixed1), textLogin, 180, 80);
    gtk_widget_show (textLogin);


    textHaslo=gtk_entry_new();
    gtk_fixed_put (GTK_FIXED (fixed1), textHaslo, 180, 150);
    gtk_widget_show (textHaslo);

    zalogujPrzycisk=gtk_button_new_with_label("Zaloguj");
     gtk_fixed_put (GTK_FIXED (fixed1), zalogujPrzycisk, 130, 220);
    gtk_widget_show (zalogujPrzycisk);
 g_signal_connect (zalogujPrzycisk, "clicked",G_CALLBACK (zaloguj), NULL);

//inicjujMenuGUI();

textCzas2=gtk_label_new ("-");
textLiczbaPunktow2=gtk_label_new ("-");
textZwyciestwo=gtk_label_new ("ZWYCIĘSTWO");
textAktualnyWyborNapis=gtk_label_new ("-");
  gtk_widget_show (window);


  gtk_main ();

}


int main( int   argc,
          char *argv[] )
{


inicjujGUI(argc,argv);



close(socketSerwer);

  return 0;
}
