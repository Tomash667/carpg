 _______  _______  _______  _______  _______
(  ____ \(  ___  )(  ____ )(  ____ )(  ____ \
| (    \/| (   ) || (    )|| (    )|| (    \/
| |      | (___) || (____)|| (____)|| |
| |      |  ___  ||     __)|  _____)| | ____
| |      | (   ) || (\ (   | (      | | \_  )
| (____/\| )   ( || ) \ \__| )      | (___) |
(_______/|/     \||/   \__/|/       (_______)

Strona: http://carpg.pl
Wersja: 0.5
Data: 2017-09-07

===============================================================================
1) Spis treœci
	1... Spis treœci
	2... Opis
	3... Sterowanie
	4... Jak graæ
	5... Tryb wieloosobowy
	6... Zmiany
	7... Komendy w konsoli
	8... Plik konfiguracyjny
	9... Linia komend
	10.. Zg³oœ b³¹d
	11.. Autorzy

===============================================================================
2) Opis
CaRpg jest to po³¹czenie gier typu action rpg/hack-n-slash jak Gothic czy
TES:Morrowind z grami rougelike SLASH'EM/Dungeon Crawl Stone Soup. W losowo
wygenerowanym œwiecie, zaczynamy w losowo wygenerowanym mieœcie, rekrutujemy
losowych bohaterów i idziemy do losowo wygenerowanych podziemi zabijaæ losowo
wygenerowanych wrogów :3 Nie chodzi o to ¿eby wszystko by³o losowe ale tak¿e
dopasowane do siebie. Jest kilka unikalnych zadañ (dok³adniej osiem) których
wykonanie jest celem w obecnej wersji. Jest dostêpny tryb multiplayer dla
maksimum 8 osób ale na razie chodzenie w tyle osób do podziemi jest trochê
niewygodne bo trzeba siê przepychaæ ¿eby kogoœ zabiæ wiêc proponuje graæ w 4
osoby. Oczekuj zmian na lepsze!

===============================================================================
3) Sterowanie - Mo¿na je zmieniæ w opcjach.
3.1. Wspólne
	Esc - menu / zamknij okno
	Alt + F4 - wyjœcie z gry
	Print Screen - zrzut ekranu (z shift bez gui)
	F5 - szybki zapis
	F9 - szybkie wczytanie
	Pause - zatrzymanie gry
	Ctrl + U - uwolnij kursor w trybie okienkowym
	~ - konsola
3.2. W grze
	Z / lewy przycisk myszki - u¿yj, atakuj, ograbiaj, rozmawiaj
	R - u¿yj, ograbiaj, rozmawiaj maj¹c wyci¹gniêt¹ broñ
	X / prawy przycisk myszki - blok
	W / strza³ka w górê - ruch do przodu
	S / strza³ka w dó³ - ruch do ty³u
	A / strza³ka w lewo - ruch w lewo
	D / strza³ka w prawo - ruch w prawo
	Q - obrót w lewo
	E - obrót w prawo
	1 - wybierz broñ do walki wrêcz i tarcze
	2 - wybierz broñ dystansow¹
	3 - u¿yj akcji
	F - automatyczny ruch do przodu
	Y - okrzyk
	TAB - ekran postaci, ekwipunek, dru¿yna
	J - dziennik
	M - minimapa
	N - okno rozmowy
	H - wypij miksturkê zdrowia
	Enter - wprowadzanie tekstu w multiplayer
	Kó³ko myszy - zmiana odleg³oœci kamery
	F2 - poka¿/ukryj fps
3.3. Ekwipunek
	lewy przycisk myszki - u¿yj, za³ó¿, kup
	prawy przycisk myszki - wyrzuæ
	klikniêcie + shift - je¿eli jest kilka przedmiotów to robi to dla wszystkich
	klikniêcie + ctrl - je¿eli jest kilka przedmiotów to robi to dla jednego
	klikniêcie + alt - wyœwietla dla ilu przedmiotów to zrobiæ
	F - zabierz wszystko
3.4. Mapa œwiata
	lewy przycisk myszki - podró¿uj do lokacji / wejdŸ
	Tab - otwórz / zamknij okno tekstu w multiplayer
	Enter - wprowadzanie tekstu w multiplayer
3.5. Dziennik
	A / strza³ka w lewo - poprzednia strona
	D / strza³ka w prawo - nastêpna strona
	W / strza³ka w górê - wyjœcie z szczegó³ów zadania
	Q - poprzednie zadanie
	E - nastêpne zadanie
	1 - zadania
	2 - plotki
	3 - notatki

===============================================================================
4) Jak graæ
* Po pierwsze przejdŸ samouczek, to tam wszystkiego siê dowiesz. Jeœli go przez
	przypadek wy³¹czy³eœ w carpg.cfg zmieñ "skip_tutorial = 1" na 0.
* Walka - Najproœciej atakowaæ wroga broni¹ do walki wrêcz. Postaraj siê ich
	otoczyæ i atakowaæ od ty³u gdy s¹ zajêci walk¹ z innymi, wtedy zadajesz
	wiêcej obra¿eñ. Blokowanie tarcz¹ nie jest aktualnie a¿ tak przydatne, nie
	otrzymujesz a¿ takich obra¿eñ ale wróg nie otrzymuje ¿adnych obra¿eñ. Jest
	on przydatna do blokowania czarów które ignoruj¹ pancerz. Strzelanie z ³uku
	jest przydatne o ile masz w dru¿ynie kogoœ kto zatrzyma wroga przed
	dojœciem do ciebie. W walce wrêcz mo¿esz wykonaæ potê¿ny atak, trzeba
	przytrzymaæ atak przez chwile, dziêki temu bêdzie silniejszy. Jest to
	przydatne przeciwko odpornym wrogom którym normalnym atakiem zadajesz ma³o
	lub nic. Atak w biegu jest jak 0.25 potê¿nego ataku ale nie mo¿esz siê
	zatrzymaæ.
* Akcje - Aby u¿yæ akcji wciœnij 3 (domyœlnie), bêdzie wtedy rysowany obszar na
	którym zostanie u¿yty ten efekt. Kliknij lewy przycisk myszki aby u¿yæ akcji,
	prawy aby anulowaæ. Czerwony obszar oznacza brak mo¿liwoœci u¿ycia akcji w
	tym miejscu.
* Wytrzyma³oœæ - Atakowanie zu¿ywa wytrzyma³oœæ, gdy siê skoñczy nie mo¿esz
	biegaæ ani atakowaæ. Blokowanie ciosów te¿ zu¿ywa wytrzyma³oœæ i gdy siê
	skoñczy blok zostaje przerwany. Przestañ atakowaæ aby odzyskaæ wytrzyma³oœæ,
	szybciej je¿eli stoisz w miejscu.
* Czêsto zapisuj grê, szczególnie na pocz¹tku kiedy jesteœ s³aby. PóŸniej przed
	walk¹ z bossem. Pamiêtaj, ¿e zawsze mo¿e siê zdarzyæ jakiœ nie naprawiony
	b³¹d i gra mo¿e siê zawiesiæ :( Przed zapisywaniem poprzedni zapis jest
	kopiowany do pliku (X.sav -> X.sav.bak) wiêc w razie problemów w czasie
	zapisywania mo¿esz go skopiowaæ.
* Pora¿ka - Gdy postaæ z twojej dru¿yny straci ca³e hp to upadnie na ziemiê.
	Gdy nie bêdzie ju¿ wrogów w okolicy to wstanie maj¹c 1 hp. Gdy wszyscy
	umr¹ nast¹pi koniec gry. Gra koñczy siê te¿ w 160 roku, twoja postaæ
	odchodzi na emeryturê.
* Podzia³ ³upów - Ka¿dy cz³onek dru¿yny otrzymuje jak¹œ czêœæ ³upów dla siebie.
	Ka¿dy bohater niezale¿ny otrzymuje 10% ³upów, reszta jest dzielona po równo
	pomiêdzy graczy. Bohaterowie którzy z jakiegoœ powodu zostali w mieœcie nic
	nie dostaj¹. Przedmioty znalezione w podziemiach s¹ oznaczone jako
	dru¿ynowe i gdy je sprzedasz zysk zostanie rozdzielony. Mo¿esz wzi¹œæ dla
	siebie przedmiot dru¿ynowy ale wtedy w przysz³oœci musisz sp³aciæ pozosta³¹
	czêœæ jego rynkowej wartoœci pozosta³ym bohaterom - jest to oznaczone jako
	kredyt. Bohaterowie niezale¿ni mog¹ sobie wzi¹æ jakiœ przedmiot jeœli
	bêdzie on lepszy ni¿ to co maj¹ aktualnie lub cie o niego poprosiæ w
	mieœcie. Mo¿esz daæ bohaterowie niezale¿nemu przedmiot, jeœli jest
	lepszy ni¿ to co ma spróbuje go od ciebie odkupiæ. Bohaterowie niezale¿ni
	przyjm¹ te¿ od ciebie ka¿d¹ miksturkê, za darmo.
* Tryb hardcore - W tym trybie po zapisaniu wychodzisz do menu a wczytanie
	usuwa zapis. Standardowy tryb dla gier rougelike. Nie jest on póki co
	zalecany bo jeœli gra siê zawiesi lub zcrashuje bêdzie trzeba zaczynaæ od
	nowa.
* Jeœli NPC zablokuje ci drogê mo¿esz na niego krzykn¹æ (domyœlnie klawisz Y)
	aby ruszy³ siê z drogi.

===============================================================================
5) Tryb wieloosobowy
* Ogólne informacje - Tryb multiplayer zosta³ przetestowany tylko na LANie.
	Nie wiadomo czy dzia³a wystarczaj¹co dobrze przez internet. Serwer musi
	mieæ publiczny adres ip lub coœ tam kombinowaæ w ustawieniach routera,
	nie znam siê na tym :( Jeœli ruch postaci laguje niech serwer pozmienia
	opcje w konsoli 'mp_interp'. Domyœlnie wynosi 0.05, podwy¿szaj j¹ a¿
	uznasz ¿e ruch postaci jest odpowiedni. Nie zapomnij siê pochwaliæ na forum
	jak posz³o! :)
* Przywódca - Bohater który ustala gdzie iœæ na mapie œwiata. Tylko on mo¿e
	zwalniaæ bohaterów i wydawaæ im rozkazy. Bohater niezale¿ny nie mo¿e zostaæ
	przywódc¹. Mo¿esz przekazaæ dowodzenie innej postaci w panelu dru¿yny TAB.
	Serwer te¿ mo¿e zmieniæ dowódcê.
* Up³yw czasu - W trybie singleplayer czas p³ynie normalnie, gdy gracz odpoczywa
	lub trenuje. W trybie multiplayer gdy jedna osoba odpoczywa/trenuje
	pozostali gracze otrzymuj¹ tyle samo wolnych dni do wykorzystania. Ile jest
	dni mo¿na zobaczyæ w panelu dru¿yny TAB. Gdy ktoœ przekroczy t¹ liczbê to
	automatycznie zwiêkszy j¹ u wszystkich. Dzieñ siê zmienia siê u wszystkich
	tylko przy przekroczeniu tej liczby. Wolnych dni ubywa w czasie podró¿y aby
	gracze nie trzymali ich w nieskoñczonoœæ.
* Zapisywanie - Nie zapisuj w trakcie walki albo w czasie jakiejœ wa¿nej
	rozmowy bo jeszcze siê coœ zepsuje. Mo¿na wczytywaæ grê tylko z menu.
* Port - Gra wykorzystuje port 37557. Jeœli jest zablokowany lub coœ ju¿ go
	u¿ywa mo¿esz go zmieniæ w pliku konfiguracyjnym.
* Aktualnie dla serwera nie ma ró¿nicy czy jest to LAN czy Internet. Klient
	mo¿e do³¹czyæ do serwera na LANie podaj¹c ip.
* Jeœli wczytywanie u klienta trwa za d³ugo mo¿e zmieniæ/dodaæ w pliku
	konfiguracyjnym wartoœæ "timeout = X" gdzie X = 1-3600 w sekundach po jakim
	czasie gracz zostanie wyrzucony.

===============================================================================
6) Zmiany
Zobacz plik changelog.txt.

===============================================================================
7) Komendy w konsoli
Aby otworzyæ konsolê wciœnij ~ [tylda na lewo od 1]. Niektóre komendy s¹
dostêpne tylko w trybie multiplayer lub tylko w lobby.
Dostêpne komendy bez trybu developera:
	cmds - wyœwietla komendy i zapisuje je do pliku commands.txt, z all wyœwietla te¿ te niedostêpne (cmds [all]).
	devmode - ustawia tryb developera (devmode 0/1).
	exit - wychodzi do menu.
	help - wyœwietla informacje o komendzie (help [komenda]).
	kick - wyrzuca gracza z serwera (kick nick).
	leader - zmienia przywódcê dru¿yny (leader nick).
	list - wyœwietla jednostki/przedmioty/zadania po id/nazwie, unit item unitn itemn quest (list typ [filtr]).
	player_devmode - w³¹cza/wy³¹cza tryb developera dla innego gracza w multiplayer (playercheat nick/all 0/1).
	quit - wychodzi z gry.
	random - losuje liczbê 1-100 lub wybiera losow¹ postaæ (random, random [warrior/hunter/rogue]).
	server - wyœwietla wiadomoœæ od serwera wszystkim graczom (say wiadomoœæ).
	version - wyœwietla wersjê gry.
	w/whisper - wysy³a prywatn¹ wiadomoœæ do gracza (whisper nick wiadomoœæ).
Pe³na lista komend w readme_eng.txt.

===============================================================================
8) Plik konfiguracyjny
W pliku konfiguracyjnym (domyœlnie carpg.cfg) mo¿na u¿ywaæ takich opcji:
	* autopick (random warrior hunter rogue) - automatycznie wybiera postaæ w
		trybie multiplayer i ustawia gotowoœæ. Dzia³a tylko raz
	* autostart (ile>=1) - automatycznie uruchamia serwer gdy jest taka liczba
		graczy
	* change_title (true [false]) - zmienia tytu³ okna w zale¿noœci od trybu
		gry
	* check_updates ([true] false) - czy sprawdzaæ czy jest nowa wersja
	* class (warrior hunter rogue) - wybrana klasa postaci w trybie szybkiej
		gry
	* con_pos_x - pozycja konsoli x
	* con_pos_y - pozycja konsoli y
	* console (true [false]) - konsola systemowa
	* crash_mode (none [normal] dataseg full) - okreœla tryb zapisywania
		informacji o crashu
	* grass_range (0-100) - zasiêg rysowania trawy
	* force_seed (true [false]) - wymuszenie okreœlonej losowoœci na ka¿dym
		poziomie
	* fullscreen ([true] false) - tryb pe³noekranowy
	* inactive_update (true [false]) - gra jest aktualizowana nawet gdy okno
		jest nieaktywne; gra jest zawsze aktualizowana w trybie multiplayer
	* log ([true] false) - logowanie do pliku
	* log_filename ["log.txt"] - plik do logowania
	* name - imiê gracza w trybie szybkiej gry
	* next_seed - nastêpne ziarno losowoœci
	* nick - zapamiêtany wybór nicku w trybie multiplayer
	* nomusic (true [false]) - po ustawieniu muzyka nie jest wczytywana i
		odtwarzana, nie mo¿na zmieniæ w czasie gry
	* nosound (true [false]) - po ustawieniu dŸwiêk nie jest wczytywany i
		odtwarzany, nie mo¿na zmieniæ w czasie gry
	* play_music ([true] false) - czy odtwarzaæ muzykê
	* play_sound ([true] false) - czy odtwarzaæ dŸwiêk
	* port ([37557]) - port w trybie multiplayer
	* quickstart (single host join joinip) - automatycznie wybierany
		tryb gry (single - gra dla jednego gracza), u¿ywa ustawieñ name (daje
		"Test" jeœli nie ma), class (losowa klasa) w multiplayer u¿ywa (nick,
		server_name, server_players, server_pswd, server_ip), jeœli nie ma
		którejœ zmiennej to nie uruchamia automatycznie
	* resolution (800x600 [1024x768]) - rozdzielczoœæ ekranu
	* screenshot_format - ustawia rozszerzenie screenshotów (jpg, bmp, tga, png)
	* seed - ziarno losowoœci
	* server_ip - zapamiêtane ip serwera
	* server_name - zapamiêtana nazwa serwera
	* server_players - zapamiêtana liczba graczy
	* server_pswd - zapamiêtane has³o serwera
	* shader_version - ustawia wersjê shadera 2/3
	* skip_tutorial (true [false]) - czy pomijaæ pytanie o samouczek
	* stream_log_file ["log.stream"] - plik do logowania informacji w mp
	* stream_log_mode (none [errors] full) - tryb logowania informacji w mp
	* timeout (1-3600) - czas oczekiwania na graczy w sekundach (domyœlnie 10)
	* vsync ([true] false) - ustawia synchronizacjê pionow¹
	* wnd_pos_x - pozycja okna x
	* wnd_pos_y - pozycja okna y
	* wnd_size_x - szerokoœæ okna
	* wnd_size_y - wysokoœæ okna

===============================================================================
9) Linia komend
To s¹ prze³¹czniki dla aplikacji, dodawane do skrótu do pliku exe.
-config plik.cfg - ustawia którego pliku cfg u¿yæ
-console - uruchamia grê z konsol¹
-delay-1 - powoduje czekanie innej gry
-delay-2 - czeka a¿ inna gra siê wczyta
-fullscreen - uruchamia w trybie pe³noekranowym
-host - automatycznie zak³ada serwer LAN
-hostip - automatycznie zak³ada serwer internet
-join - automatycznie do³¹cza do serwera LAN
-joinip - automatycznie do³¹cza do serwera w internecie
-nomusic - brak muzyki
-nosound - brak dŸwiêku
-single - rozpoczyna szybk¹ grê w trybie jednego gracza
-windowed - uruchamia w trybie okienkowym
-test - testuje dane gry

===============================================================================
10) Zg³oœ b³¹d
B³êdy mo¿esz a wrêcz powinieneœ zg³aszaæ na stronie http://carpg.pl w
odpowiednim dziale forum. Nie zapomnij podaæ wszystkich mo¿liwych szczegó³ów
które mog¹ pomóc w jego naprawieniu. Jeœli wyskoczy ¿e utworzono plik
minidump to go do³¹cz. Przydatny te¿ bêdzie plik log.txt i zapis przed
zawieszeniem siê o ile siê zapisywa³eœ. Przed zg³oszeniem b³êdu sprawdŸ czy ktoœ
ju¿ go nie zg³osi³ w odpowiednim temacie.

===============================================================================
110) Autorzy
Tomashu - Programowanie, modele, tekstury, pomys³y, fabu³a.
Leinnan - Modele, tekstury, pomys³y, testowanie.
MarkK - Modele i tekstury jedzenia oraz innych obiektów.
Zielu - Niechêtne testowanie.

Podziêkowania za znalezione b³êdy:
	darktorq
	Docucat
	fire
	Medarc
	Minister of Death
	Paradox Edge
	thebard88
	Vinur_Gamall
	XNautPhD
	Zettaton
	Zinny

Modele:
	* kaangvl - fish
	* yd - sausage
Tekstury:
	* http://cgtextures.com
	* Cube
	* texturez.com
	* texturelib.com
	* SwordAxeIcon by Raindropmemory
	* Ikony klas - http://game-icons.net/
Muzyka:
	* £ukasz Xwokloiz
	* Celestial Aeon Project
	* Project Divinity
	* For Originz - Kevin MacLeod (incompetech.com)
	* http://www.deceasedsuperiortechnician.com/
DŸwiêki:
	* http://www.freesound.org/
	* http://www.pacdv.com/sounds/
	* http://www.soundjay.com
	* http://www.grsites.com/archive/sounds/
	* http://www.wavsource.com
