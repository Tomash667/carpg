 _______  _______  _______  _______  _______
(  ____ \(  ___  )(  ____ )(  ____ )(  ____ \
| (    \/| (   ) || (    )|| (    )|| (    \/
| |      | (___) || (____)|| (____)|| |
| |      |  ___  ||     __)|  _____)| | ____
| |      | (   ) || (\ (   | (      | | \_  )
| (____/\| )   ( || ) \ \__| )      | (___) |
(_______/|/     \||/   \__/|/       (_______)

Strona: https://carpg.pl
Wersja: 0.19
Data: 2022-12-09

===================================================================================================
1) Spis treści
	1... Spis treści
	2... Opis
	3... Sterowanie
	4... Jak grać
	5... Tryb wieloosobowy
	6... Zmiany
	7... Komendy w konsoli
	8... Plik konfiguracyjny
	9... Linia komend
	10.. Zgłoś błąd
	11.. Autorzy

===================================================================================================
2) Opis
CaRpg jest to połączenie gier typu action rpg/hack-n-slash jak Gothic czy TES:Morrowind z grami
roguelike SLASH'EM/Dungeon Crawl Stone Soup. W losowo wygenerowanym świecie, zaczynamy w losowo
wygenerowanym mieście, rekrutujemy losowych bohaterów i idziemy do losowo wygenerowanych podziemi
zabijać losowo wygenerowanych wrogów :3 Nie chodzi o to żeby wszystko było losowe ale także
dopasowane do siebie. Jest kilka unikalnych zadań (dokładniej osiem) których wykonanie jest celem
w obecnej wersji. Jest dostępny tryb multiplayer dla maksimum 8 osób ale na razie chodzenie w tyle
osób do podziemi jest trochę niewygodne bo trzeba się przepychać żeby kogoś zabić więc proponuje
grać w 4 osoby. Oczekuj zmian na lepsze!

===================================================================================================
3) Sterowanie - Można je zmienić w opcjach.
3.1. Wspólne
	Esc - menu / zamknij okno
	Alt + F4 - wyjście z gry
	Print Screen - zrzut ekranu (z shift bez gui)
	F5 - szybki zapis
	F9 - szybkie wczytanie
	Pause - zatrzymanie gry
	Ctrl + U - uwolnij kursor w trybie okienkowym
	~ - konsola
3.2. W grze
	E - użyj, ograbiaj, rozmawiaj mając wyciągniętą broń
	lewy przycisk myszki - użyj, atakuj, ograbiaj, rozmawiaj
	Z / 4 przycisk myszki - druga opcja ataku
	X - anuluj atak
	prawy przycisk myszki - blok
	W - ruch do przodu
	S - ruch do tyłu
	A - ruch w lewo
	D - ruch w prawo
	1..0 - skróty przedmiotów/zdolności
	F - automatyczny ruch do przodu
	Caps Lock - przełącz bieganie/chodzenie
	Y - okrzyk
	C - ekran postaci
	I - ekwipunek
	T - drużyna
	K - zdolności
	J - dziennik
	Tab - minimapa
	'" - okno rozmowy
	Enter - wprowadzanie tekstu w multiplayer
	Kółko myszy - zmiana odległości kamery
	F1 - zatwierdź powiadomienie
	F3 - odrzuć powiadomienie
	Ctrl + F1 - pokaż/ukryj fps
	Ctrl + F2 - pokaż/ukryj informacje diagnostyczne
3.3. Ekwipunek
	lewy przycisk myszki - użyj, załóż, kup
	prawy przycisk myszki - wyrzuć
	kliknięcie + shift - jeżeli jest kilka przedmiotów to robi to dla wszystkich
	kliknięcie + ctrl - jeżeli jest kilka przedmiotów to robi to dla jednego
	kliknięcie + alt - wyświetla dla ilu przedmiotów to zrobić
	F - zabierz wszystko
3.4. Mapa świata
	lewy przycisk myszki - podróżuj do lokalizacji / wejdź
	Tab - otwórz / zamknij okno tekstu w multiplayer
	Enter - wprowadzanie tekstu w multiplayer
	F - szukaj lokalizacji
3.5. Dziennik
	A / strzałka w lewo - poprzednia strona
	D / strzałka w prawo - następna strona
	W / strzałka w górę - wyjście z szczegółów zadania
	Q - poprzednie zadanie
	E - następne zadanie
	1 - zadania
	2 - plotki
	3 - notatki

===================================================================================================
4) Jak grać
* Po pierwsze przejdź samouczek, to tam wszystkiego się dowiesz. Jeśli go przez przypadek
	wyłączyłeś w carpg.cfg zmień "skipTutorial = 1" na 0.
* Walka - Najprościej atakować wroga bronią do walki wręcz. Postaraj się ich otoczyć i atakować od
	tyłu gdy są zajęci walką z innymi, wtedy zadajesz więcej obrażeń. Blokowanie tarczą nie jest
	aktualnie aż tak przydatne, nie otrzymujesz aż takich obrażeń ale wróg nie otrzymuje żadnych
	obrażeń. Jest on przydatna do blokowania czarów które ignorują pancerz. Strzelanie z łuku jest
	przydatne o ile masz w drużynie kogoś kto zatrzyma wroga przed dojściem do ciebie. W walce
	wręcz możesz wykonać potężny atak, trzeba przytrzymać atak przez chwile, dzięki temu będzie
	silniejszy. Jest to przydatne przeciwko odpornym wrogom którym normalnym atakiem zadajesz mało
	lub nic. Atak w biegu jest jak 0.25 potężnego ataku ale nie możesz się zatrzymać.
* Zdolności - Aby użyć zdolności wciśnij 3 (domyślnie), będzie wtedy rysowany obszar na którym
	zostanie użyty ten efekt. Kliknij lewy przycisk myszki aby użyć zdolności, prawy aby anulować.
	Czerwony obszar oznacza brak możliwości użycia zdolności w tym miejscu. Niektóre czary możesz
	rzucić na siebie przytrzymując ruch do tyłu, gracz zostanie podświetlony.
* Wytrzymałość - Atakowanie zużywa wytrzymałość, gdy się skończy nie możesz biegać ani atakować.
	Blokowanie ciosów też zużywa wytrzymałość i gdy się skończy blok zostaje przerwany. Przestań
	atakować aby odzyskać wytrzymałość, szybciej jeżeli stoisz w miejscu.
* Często zapisuj grę, szczególnie na początku kiedy jesteś słaby. Później przed walką z bossem.
	Pamiętaj, że zawsze może się zdarzyć jakiś nie naprawiony błąd i gra może się zawiesić :( Przed
	zapisywaniem poprzedni zapis jest kopiowany do pliku (X.sav -> X.sav.bak) więc w razie
	problemów w czasie zapisywania możesz go skopiować.
* Porażka - Gdy postać z twojej drużyny straci całe hp to upadnie na ziemię. Gdy nie będzie już
	wrogów w okolicy to wstanie mając 1 hp. Gdy wszyscy umrą nastąpi koniec gry. Gra kończy się też
	gdy twoja postać odchodzi na emeryturę.
* Podział łupów - Każdy członek drużyny otrzymuje jakąś część łupów dla siebie. Każdy bohater
	niezależny otrzymuje 10% łupów, reszta jest dzielona po równo pomiędzy graczy. Bohaterowie
	którzy z jakiegoś powodu zostali w mieście nic nie dostają. Przedmioty znalezione w podziemiach
	są oznaczone jako drużynowe i gdy je sprzedasz zysk zostanie rozdzielony. Możesz wziąść dla
	siebie przedmiot drużynowy ale wtedy w przyszłości musisz spłacić pozostałą część jego rynkowej
	wartości pozostałym bohaterom - jest to oznaczone jako kredyt. Bohaterowie niezależni mogą
	sobie wziąć jakiś przedmiot jeśli będzie on lepszy niż to co mają aktualnie lub cie o niego
	poprosić w mieście. Możesz dać bohaterowie niezależnemu przedmiot, jeśli jest lepszy niż to co
	ma spróbuje go od ciebie odkupić. Bohaterowie niezależni przyjmą też od ciebie każdą miksturkę,
	za darmo.
* Tryb hardcore - W tym trybie po zapisaniu wychodzisz do menu a wczytanie usuwa zapis. Standardowy
	tryb dla gier roguelike. Nie jest on póki co zalecany bo jeśli gra się zawiesi lub zcrashuje
	będzie trzeba zaczynać od nowa.
* Jeśli NPC zablokuje ci drogę możesz na niego krzyknąć (domyślnie klawisz Y) aby ruszył się z
	drogi.

===================================================================================================
5) Tryb wieloosobowy
* Ogólne informacje - Tryb multiplayer został przetestowany tylko na LANie. Nie wiadomo czy działa
	wystarczająco dobrze przez internet. Dzięki głównemu serwerowi można się połączyć pomiędzy
	dwoma komputerami za routerem. Jeśli jednak się to nie powiedzie trzeba odpowiednio
	skonfigurować router (ustawienia NAT). Jeśli ruch postaci laguje niech serwer pozmienia opcje w
	konsoli 'mpInterp'. Domyślnie wynosi 0.05, podwyższaj ją aż uznasz że ruch postaci jest
	odpowiedni. Nie zapomnij się pochwalić na forum jak poszło! :)
* Przywódca - Bohater który ustala gdzie iść na mapie świata. Tylko on może zwalniać bohaterów i
	wydawać im rozkazy. Bohater niezależny nie może zostać przywódcą. Możesz przekazać dowodzenie
	innej postaci w panelu drużyny (domyślny skrót T). Serwer też może zmienić dowódcę.
* Upływ czasu - W trybie singleplayer czas płynie normalnie, gdy gracz odpoczywa lub trenuje. W
	trybie multiplayer gdy jedna osoba odpoczywa/trenuje pozostali gracze otrzymują tyle samo
	wolnych dni do wykorzystania. Ile jest dni można zobaczyć w panelu drużyny (domyślny skrót T).
	Gdy ktoś przekroczy tą liczbę to automatycznie zwiększy ją u wszystkich. Dzień się zmienia się
	u wszystkich tylko przy przekroczeniu tej liczby. Wolnych dni ubywa w czasie podróży aby gracze
	nie trzymali ich w nieskończoność.
* Zapisywanie - Nie zapisuj w trakcie walki albo w czasie jakiejś ważnej rozmowy bo jeszcze się coś
	zepsuje.
* Jeśli wczytywanie u klienta trwa za długo może zmienić/dodać w pliku konfiguracyjnym wartość
	"timeout = X" gdzie X = 1-3600 w sekundach po jakim czasie gracz zostanie wyrzucony.
* Porty - gra wykorzystuje port udp 37557, można go zmienić w pliku konfiguracyjnym. Główny serwer
	wykorzystuje porty tcp 8080 i udp 60481 do komunikacji.

===================================================================================================
6) Zmiany
Zobacz plik changelog_polish.txt.

===================================================================================================
7) Komendy w konsoli
Aby otworzyć konsolę wciśnij ~ [tylda na lewo od 1]. Niektóre komendy są dostępne tylko w trybie
	multiplayer lub tylko w lobby. Dostępne komendy bez trybu developera:
	cmds - wyświetla komendy i zapisuje je do pliku commands.txt, z all wyświetla też te niedostępne (cmds [all]).
	devmode - ustawia tryb developera (devmode 0/1).
	exit - wychodzi do menu.
	help - wyświetla informacje o komendzie (help [komenda]).
	list - wyświetla jednostki/przedmioty/zadania po id/nazwie, unit item unitn itemn quest (list typ [filtr]).
	playerDevmode - włącza/wyłącza tryb developera dla innego gracza w multiplayer (playerDevmode nick/all 0/1).
	quit - wychodzi z gry.
	random - losuje liczbę 1-100 lub wybiera losową postać (random, random [warrior/hunter/rogue]).
	version - wyświetla wersję gry.
	w/whisper - wysyła prywatną wiadomość do gracza (whisper nick wiadomość).
Pełna lista komend w readme_english.txt.

===================================================================================================
8) Plik konfiguracyjny
W pliku konfiguracyjnym (domyślnie carpg.cfg) można używać takich opcji:
	* autopick (random warrior hunter rogue) - automatycznie wybiera postać w trybie multiplayer
		i ustawia gotowość. Działa tylko raz
	* autostart (ile>=1) - automatycznie uruchamia serwer gdy jest taka liczba graczy
	* changeTitle (true [false]) - zmienia tytuł okna w zależności od trybu gry
	* checkUpdates ([true] false) - czy sprawdzać czy jest nowa wersja
	* class (warrior hunter rogue) - wybrana klasa postaci w trybie szybkiej gry
	* conPos Int2 - pozycja konsoli x, y
	* conSize Int2 - rozmiar konsoli x, y
	* console (true [false]) - konsola systemowa
	* crashMode (none [normal] dataseg full) - określa tryb zapisywania informacji o crashu
	* grassRange (0-100) - zasięg rysowania trawy
	* featureLevel ("10.0" "10.1" "11.0") - poziom funkcji directx
	* fullscreen ([true] false) - tryb pełnoekranowy
	* inactiveUpdate (true [false]) - gra jest aktualizowana nawet gdy okno jest nieaktywne; gra
		jest zawsze aktualizowana w trybie multiplayer
	* log ([true] false) - logowanie do pliku
	* logFilename ["log.txt"] - plik do logowania
	* name - imię gracza w trybie szybkiej gry
	* nextSeed - następne ziarno losowości
	* nick - zapamiętany wybór nicku w trybie multiplayer
	* nosound (true [false]) - po ustawieniu muzyka i dźwięki nie jest wczytywane i odtwarzane, nie
		można zmienić w czasie gry
	* port ([37557]) - port w trybie multiplayer
	* quickstart (single host join joinip) - automatycznie wybierany tryb gry (single - gra dla
		jednego gracza), używa ustawień name (daje "Test" jeśli nie ma), class (losowa klasa) w
		multiplayer używa (nick, serverName, serverPlayers, serverPswd, serverIp), jeśli nie ma
		którejś zmiennej to nie uruchamia automatycznie
	* resolution (800x600 [1024x768]) - rozdzielczość ekranu
	* screenshotFormat - ustawia rozszerzenie screenshotów (jpg, bmp, tif, gif, png, dds)
	* serverIp - zapamiętane ip serwera
	* serverLan - jeśli jest 1 to serwer nie będzie rejestrowany w głównym serwerze
	* serverName - zapamiętana nazwa serwera
	* serverPlayers - zapamiętana liczba graczy
	* serverPswd - zapamiętane hasło serwera
	* skipTutorial (true [false]) - czy pomijać pytanie o samouczek
	* timeout (1-3600) - czas oczekiwania na graczy w sekundach (domyślnie 10)
	* vsync ([true] false) - ustawia synchronizację pionową
	* wndPos Int2 - pozycja okna x, y
	* wndSize Int2 - wymiary okna x, y

===================================================================================================
9) Linia komend
To są przełączniki dla aplikacji, dodawane do skrótu do pliku exe. Oprócz wszystkich opcji z pliku
konfiguracyjnego można też zmienić z którego pliku korzystać: -config plik.cfg - ustawia którego
pliku cfg użyć.

===================================================================================================
10) Zgłoś błąd 🐜
Błędy możesz a wręcz powinieneś zgłaszać na stronie https://carpg.pl w odpowiednim dziale forum.
Nie zapomnij podać wszystkich możliwych szczegółów które mogą pomóc w jego naprawieniu. Jeśli
wyskoczy że utworzono plik minidump to go dołącz. Przydatny też będzie plik log.txt i zapis przed
zawieszeniem się o ile się zapisywałeś. Przed zgłoszeniem błędu sprawdź czy ktoś już go nie zgłosił
w odpowiednim temacie.

===================================================================================================
110) Autorzy
Tomashu - Programowanie, modele, tekstury, pomysły, fabuła.
Leinnan - Modele, tekstury, pomysły, testowanie.
MarkK - Modele i tekstury jedzenia oraz innych obiektów.
Groszek - Modele, tekstury i gui.
Shdorsh - Poprawki w angielskim tłumaczeniu.
Zielu - Testowanie i modele.
BottledByte - Zgłaszanie błedów i usprawnienia w kodzie.

❤ Podziękowania za testy i znalezione błędy:
	Ampped
	AnimeIsWrong
	darktorq
	Docucat
	fire
	Groszek
	Harorri
	Lemiczek
	Medarc
	MikelkCZ
	MildlyPhilosophicalHobbit
	Minister of Death
	mishka
	Mokai
	Paradox Edge
	Savagesheep
	SwagOfficerSuccubus
	thebard88
	Vinur_Gamall
	Woltvint
	XNautPhD
	xweert123
	Zettaton
	Zinny
🎵 Audio Engine: FMOD Studio by Firelight Technologies.
🗃 Modele:
	* https://opengameart.org
		* CDmir - chest
		* Clint Bellanger - naraphim sword
		* Enetheru - windmill
		* Grefuntor - slime
		* johndh - cutlass
		* kindland - shield of death
		* Lucian Pavel - short sword, kite shield
		* Mophs - scarecrow
		* nubux - ogre
		* p0ss - long sword
		* rubberduck - shrine
		* sandsound - morningstar
		* System G6 - shield
		* TiZiana - vodka, gray potion
		* Wind astella - battle axe
	* kaangvl - fish
	* yd - sausage
🎨 Tekstury:
	* https://textures.com
	* Cube
	* texturez.com
	* texturelib.com
	* SwordAxeIcon by Raindropmemory
	* Ikony klas - https://game-icons.net/
🎧 Muzyka:
	* Łukasz Xwokloiz
	* Celestial Aeon Project
	* Project Divinity
	* HorrorPen - Winds of Stories
	* For Originz - Kevin MacLeod (incompetech.com)
	* Ruined City Theme - Music by Cleyton Kauffman - https://soundcloud.com/cleytonkauffman
	* http://www.deceasedsuperiortechnician.com/
🔈 Dźwięki:
	* https://opengameart.org
		* artisticdude - ogre sounds
		* OwlishMedia - coughs
		* Little Robot Sound Factory (www.littlerobotsoundfactory.com) - splash
	* https://www.freesound.org/
		* DrMinky - Slime Death, Slime Land
		* InspectorJ (www.jshaw.co.uk) - Boiling Water, Large
		* newagesoup - wolf growl
		* wubitog - Slime attack or movement
	* https://www.pacdv.com/sounds/
	* https://www.soundjay.com
	* http://www.grsites.com/archive/sounds/
	* http://www.wavsource.com
