#pragma once

//-----------------------------------------------------------------------------
// identyfikatory questów
// kolejnoœæ nie jest nigdzie u¿ywana, mo¿na dawaæ jak siê chce, ale na koñcu ¿eby zapisy by³y kompatybilne
enum QUEST
{
	Q_KOPALNIA, // po oczyszczeniu kopalni zysk 1000 sz/m, po jakimœ czasie kurier, ¿e odkryto ¿y³ê z³ota i po zainwestowaniu 100k daje 2500 sz/m, po jakimœ czasie dokopuj¹ siê do portalu który przenosi do miejsca gdzie jest artefakt
	Q_TARTAK, // trzeba oczyœciæ las z potworów i od tego czasu przynosi zysk 500 sz/m
	Q_BANDYCI, // pomagasz mistrzowi agentów przeciwko bandytom
	Q_MAGOWIE, // najpierw jakiœ quest ¿e magowie zachodz¹ ci za skórê, potem przez przypadek im pomagasz,
	Q_MAGOWIE2,
	Q_ORKOWIE,
	Q_ORKOWIE2,
	Q_GOBLINY,
	Q_ZLO,

	Q_DOSTARCZ_LIST, // burmistrz, dostarcz list do innego burmistrza
	Q_DOSTARCZ_PACZKE, // burmistrz, idŸ do innego miasta i odbierz paczkê od burmistrza i dostarcz do tego
	Q_ROZNIES_WIESCI, // burmistrz, zanieœ wieœci do okolicznych burmistrzów
	Q_URATUJ_PORWANA_OSOBE, // stra¿nik, bandyci porwali osobê, udaj siê do podziemi i j¹ uratuj
	Q_BANDYCI_POBIERAJA_OPLATE, // stra¿nik, bandyci rozbili obóz ko³o drogi i pobieraj¹ op³atê, zabij ich
	Q_OBOZ_KOLO_MIASTA, // stra¿nik, bandyci rozbili obóz niedaleko miasta, trzeba ich pokonaæ
	Q_ODZYSKAJ_PACZKE, // burmistrz, bandyci przejeli paczkê, trzeba j¹ odzyskaæ
	Q_ZABIJ_ZWIERZETA, // stra¿nik, w lesie/jaskini s¹ zwierzêta ko³o miasta, trzba je pokonaæ
	Q_ZGUBIONY_PRZEDMIOT, // tawerna, ktoœ zgubi³ przedmiot w podziemiach i chce go odzyskaæ
	Q_UKRADZIONY_PRZEDMIOT, // tawerna, bandyci coœ ukradli, trzeba to odzyskaæ
	Q_PRZYNIES_ARTEFAKT, // tawerna, poszukuje jakiegoœ artefaktu, jest w podziemiach/krypcie

	Q_SZALENI,
	Q_LIST_GONCZY,

	Q_MAIN,
};

//-----------------------------------------------------------------------------
static const int UNIQUE_QUESTS = 8;
