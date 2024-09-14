/* ************************************************************************
*   File: names.cpp                                     Part of Bylins    *
*  Usage: saving\checked names , agreement and disagreement               *
*                                                                         *
*  17.06.2003 - made by Alez                                              *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

//#include "interpreter.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "engine/entities/char_player.h"

#include "names.h"

namespace NewNames {
static void save();
static void cache_add(CharData *ch);
}
extern void send_to_gods(char *text, bool demigod);

// Check if name agree (name must be parsed)
int was_agree_name(DescriptorData *d) {
	log("was_agree_name start");
	FILE *fp;
	char temp[kMaxInputLength];
	char immname[kMaxInputLength];
	char mortname[6][kMaxInputLength];
	int immlev;
	int sex;
	int i;

//1. Load list

	if (!(fp = fopen(ANAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for reading");
		log("was_agree_name end");
		return (1);
	}
//2. Find name in list ...
	while (get_line(fp, temp)) {
		// Format First name/pad1/pad2/pad3/pad4/pad5/sex immname immlev
		sscanf(temp, "%s %s %s %s %s %s %d %s %d", mortname[0],
			   mortname[1], mortname[2], mortname[3], mortname[4], mortname[5], &sex, immname, &immlev);
		if (!strcmp(mortname[0], GET_NAME(d->character))) {
			// We find char ...
			for (i = 1; i < 6; i++) {
				d->character->player_data.PNames[i] = std::string(mortname[i]);
			}
			d->character->set_sex(static_cast<EGender>(sex));
			// Auto-Agree char ...
			NAME_GOD(d->character) = immlev + 1000;
			NAME_ID_GOD(d->character) = GetPlayerIdByName(immname);
			sprintf(buf, "\r\nВаше имя одобрено!\r\n");
			SEND_TO_Q(buf, d);
			sprintf(buf, "AUTOAGREE: %s was agreed by %s", GET_PC_NAME(d->character), immname);
			log(buf, d);
			fclose(fp);
			log("was_agree_name end");
			return (0);
		}
	}
	fclose(fp);
	log("was_agree_name end");
	return (1);
}

int was_disagree_name(DescriptorData *d) {
	log("was_disagree_name start");
	FILE *fp;
	char temp[kMaxInputLength];
	char mortname[kMaxInputLength];
	char immname[kMaxInputLength];
	int immlev;

	if (!(fp = fopen(DNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for reading");
		log("was_disagree_name end");
		return (1);
	}
	//1. Load list
	//2. Find name in list ...
	//3. Compare names ... get next
	while (get_line(fp, temp)) {
		// Extract entities and
		sscanf(temp, "%s %s %d", mortname, immname, &immlev);
		if (!strcmp(mortname, GET_NAME(d->character))) {
			// Char found all ok;

			sprintf(buf, "\r\nВаше имя запрещено!\r\n");
			SEND_TO_Q(buf, d);
			sprintf(buf, "AUTOAGREE: %s was disagreed by %s", GET_PC_NAME(d->character), immname);
			log(buf, d);

			fclose(fp);
			log("was_disagree_name end");
			return (0);
		};
	}
	fclose(fp);
	log("was_disagree_name end");
	return (1);
}

void rm_agree_name(CharData *d) {
	FILE *fin;
	FILE *fout;
	char temp[kMaxInputLength];
	char immname[kMaxInputLength];
	char mortname[6][kMaxInputLength];
	int immlev;
	int sex;

	// 1. Find name ...
	if (!(fin = fopen(ANAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" ANAME_FILE ".tmp", "w"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp)) {
		// Get name ...
		sscanf(temp, "%s %s %s %s %s %s %d %s %d", mortname[0],
			   mortname[1], mortname[2], mortname[3], mortname[4], mortname[5], &sex, immname, &immlev);
		if (strcmp(mortname[0], GET_NAME(d))) {
			// Name un matches ... do copy ...
			fprintf(fout, "%s\n", temp);
		};
	}

	fclose(fin);
	fclose(fout);
	// Rewrite from tmp
	rename(ANAME_FILE ".tmp", ANAME_FILE);
}

// список неодобренных имен, дубль2
// на этот раз ничего перебирать не будем, держа все в памяти и обновляя по необходимости

struct NewName {
	std::string name0; // падежи
	std::string name1; // --//--
	std::string name2; // --//--
	std::string name3; // --//--
	std::string name4; // --//--
	std::string name5; // --//--
	std::string email; // мыло
	EGender sex{EGender::kNeutral};         // часто не ясно, для какоо пола падежи вообще
};

typedef std::shared_ptr<NewName> NewNamePtr;
typedef std::map<std::string, NewNamePtr> NewNameListType;

static NewNameListType NewNameList;

// сохранение списка в файл
static void NewNames::save() {
	std::ofstream file(NNAME_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", NNAME_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	for (NewNameListType::const_iterator it = NewNameList.begin(); it != NewNameList.end(); ++it)
		file << it->first << "\n";

	file.close();
}

// добавить в список без сохранения на диск
static void NewNames::cache_add(CharData *ch) {
	NewNamePtr name(new NewName);

	name->name0 = ch->get_name();
	name->name1 = GET_PAD(ch, 1);
	name->name2 = GET_PAD(ch, 2);
	name->name3 = GET_PAD(ch, 3);
	name->name4 = GET_PAD(ch, 4);
	name->name5 = GET_PAD(ch, 5);
	name->email = GET_EMAIL(ch);
	name->sex = GET_SEX(ch);

	NewNameList[GET_NAME(ch)] = name;
}

// добавление имени в список неодобренных для показа иммам
void NewNames::add(CharData *ch) {
	cache_add(ch);
	save();
}

// поиск/удаление персонажа из списка неодобренных имен
void NewNames::remove(CharData *ch) {
	NewNameListType::iterator it;
	it = NewNameList.find(GET_NAME(ch));
	if (it != NewNameList.end())
		NewNameList.erase(it);
	save();
}

// для удаления через команду имма
void NewNames::remove(const std::string &name, CharData *actor) {
	auto it = NewNameList.find(name);
	if (it != NewNameList.end()) {
		NewNameList.erase(it);
		SendMsgToChar("Запись удалена.\r\n", actor);
	} else
		SendMsgToChar("В списке нет такого имени.\r\n", actor);

	save();
}

// лоад списка неодобренных имен
void NewNames::load() {
	std::ifstream file(NNAME_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", NNAME_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer;
	while (file >> buffer) {
		// сразу проверяем не сделетился ли уже персонаж
		Player t_tch;
		Player *tch = &t_tch;
		if (LoadPlayerCharacter(buffer.c_str(), tch, ELoadCharFlags::kFindId) < 0)
			continue;
		// не сделетился...
		cache_add(tch);
	}

	file.close();
	save();
}

// вывод списка неодобренных имму
bool NewNames::show(CharData *actor) {
	if (NewNameList.empty())
		return false;

	std::ostringstream buffer;
	buffer << "\r\nИгроки, ждущие одобрения имени (имя <игрок> одобрить/запретить/удалить):\r\n" << CCWHT(actor, C_NRM);
	for (NewNameListType::const_iterator it = NewNameList.begin(); it != NewNameList.end(); ++it) {
		const auto sex = static_cast<size_t>(to_underlying(it->second->sex));
		buffer << "Имя: " << it->first << " " << it->second->name0 << "/" << it->second->name1
			   << "/" << it->second->name2 << "/" << it->second->name3 << "/" << it->second->name4
			   << "/" << it->second->name5 << " Email: &S"
			   << (GET_GOD_FLAG(actor, EGf::kDemigod) ? "неопределен" : it->second->email)
			   << "&s Пол: " << genders[sex] << "\r\n";
	}
	buffer << CCNRM(actor, C_NRM);
	SendMsgToChar(buffer.str(), actor);
	return true;
}

// Name auto-agreement
int NewNames::auto_authorize(DescriptorData *d) {
	// Check for name ...
	if (!was_agree_name(d))
		return AUTO_ALLOW;

	if (!was_disagree_name(d))
		return AUTO_BAN;

	return NO_DECISION;
}

static void rm_disagree_name(CharData *d) {
	FILE *fin;
	FILE *fout;
	char temp[256];
	char immname[256];
	char mortname[256];
	int immlev;

	// 1. Find name ...
	if (!(fin = fopen(DNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" DNAME_FILE ".tmp", "w"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp)) {
		// Get name ...
		sscanf(temp, "%s %s %d", mortname, immname, &immlev);
		if (strcmp(mortname, GET_NAME(d)) != 0) {
			// Name un matches ... do copy ...
			fprintf(fout, "%s\n", temp);
		};
	}
	fclose(fin);
	fclose(fout);
	rename(DNAME_FILE ".tmp", DNAME_FILE);
}

static void add_agree_name(CharData *d, const char *immname, int immlev) {
	FILE *fl;
	if (!(fl = fopen(ANAME_FILE, "a"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl,
			"%s %s %s %s %s %s %d %s %d\r\n",
			GET_NAME(d),
			GET_PAD(d, 1),
			GET_PAD(d, 2),
			GET_PAD(d, 3),
			GET_PAD(d, 4),
			GET_PAD(d, 5),
			static_cast<int>(GET_SEX(d)),
			immname,
			immlev);
	fclose(fl);
}

static void add_disagree_name(CharData *d, const char *immname, int immlev) {
	FILE *fl;
	if (!(fl = fopen(DNAME_FILE, "a"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl, "%s %s %d\r\n", GET_NAME(d), immname, immlev);
	fclose(fl);
}

static void disagree_name(CharData *d, const char *immname, int immlev) {
	// Clean record from agreed if present ...
	rm_agree_name(d);
	rm_disagree_name(d);
	NewNames::remove(d);
	// Add record to disagreed if not present ...
	add_disagree_name(d, immname, immlev);
}

void agree_name(CharData *d, const char *immname, int immlev) {
	// Clean record from disgreed if present ...
	rm_agree_name(d);
	rm_disagree_name(d);
	NewNames::remove(d);
	// Add record to agreed if not present ...
	add_agree_name(d, immname, immlev);
}

enum { NAME_AGREE, NAME_DISAGREE, NAME_DELETE };

static void go_name(CharData *ch, CharData *vict, int action) {
	int god_level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);

	if (GetRealLevel(vict) > god_level) {
		SendMsgToChar("А он ведь старше вас...\r\n", ch);
		return;
	}

	// одобряем или нет
	int lev = NAME_GOD(vict);
	if (lev > 1000)
		lev = lev - 1000;
	if (lev > god_level) {
		SendMsgToChar("Об этом имени уже позаботился бог старше вас.\r\n", ch);
		return;
	}

	if (lev == god_level)
		if (NAME_ID_GOD(vict) != GET_IDNUM(ch))
			SendMsgToChar("Об этом имени уже позаботился другой бог вашего уровня.\r\n", ch);

	if (action == NAME_AGREE) {
		NAME_GOD(vict) = god_level + 1000;
		NAME_ID_GOD(vict) = GET_IDNUM(ch);
		//SendMsgToChar("Имя одобрено!\r\n", ch);
		SendMsgToChar(vict, "&GВаше имя одобрено!&n\r\n");
		agree_name(vict, GET_NAME(ch), god_level);
		sprintf(buf, "&c%s одобрил%s имя игрока %s.&n\r\n", GET_NAME(ch), GET_CH_SUF_1(ch), GET_NAME(vict));
		send_to_gods(buf, true);
		// В этом теперь нет смысла
		//mudlog(buf, CMP, kLevelGod, SYSLOG, true);

	} else {
		NAME_GOD(vict) = god_level;
		NAME_ID_GOD(vict) = GET_IDNUM(ch);
		//SendMsgToChar("Имя запрещено!\r\n", ch);
		SendMsgToChar(vict, "&RВаше имя запрещено!&n\r\n");
		disagree_name(vict, GET_NAME(ch), god_level);
		sprintf(buf, "&c%s запретил%s имя игрока %s.&n\r\n", GET_NAME(ch), GET_CH_SUF_1(ch), GET_NAME(vict));
		send_to_gods(buf, true);
		//mudlog(buf, CMP, kLevelGod, SYSLOG, true);

	}

}

const char *MORTAL_DO_TITLE_FORMAT = "\r\n"
									 "имя - вывод списка имен, ждущих одобрения, если они есть\r\n"
									 "имя <игрок> одобрить - одобрить имя данного игрока\r\n"
									 "имя <игрок> запретить - запретить имя данного игрока\r\n"
									 "имя <игрок> удалить - удалить данное имя из списка без запрета или одобрения\r\n";

void do_name(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	std::string name, command = argument;
	GetOneParam(command, name);

	if (name.empty()) {
		if (!NewNameList.empty())
			NewNames::show(ch);
		else
			SendMsgToChar(MORTAL_DO_TITLE_FORMAT, ch);
		return;
	}

	utils::Trim(command);
	int action = -1;
	if (CompareParam(command, "одобрить"))
		action = NAME_AGREE;
	else if (CompareParam(command, "запретить"))
		action = NAME_DISAGREE;
	else if (CompareParam(command, "удалить"))
		action = NAME_DELETE;

	if (action < 0) {
		SendMsgToChar(MORTAL_DO_TITLE_FORMAT, ch);
		return;
	}

	name_convert(name);
	if (action == NAME_DELETE) {
		NewNames::remove(name, ch);
		return;
	}

	CharData *vict;
	if ((vict = get_player_vis(ch, name, EFind::kCharInWorld)) != nullptr) {
		if (!(vict = get_player_pun(ch, name, EFind::kCharInWorld))) {
			SendMsgToChar("Нет такого игрока.\r\n", ch);
			return;
		}
		go_name(ch, vict, action);
	} else {
		vict = new Player; // TODO: переделать на стек
		if (LoadPlayerCharacter(name.c_str(), vict, ELoadCharFlags::kFindId) < 0) {
			SendMsgToChar("Такого персонажа не существует.\r\n", ch);
			delete vict;
			return;
		}
		go_name(ch, vict, action);
		vict->save_char();
		delete vict;
	}
}

/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

const int kMaxInvalidNames{200};
char *invalid_list[kMaxInvalidNames];
std::size_t num_invalid{0};

int IsValidName(char *newname) {
	if (!*invalid_list || num_invalid < 1) {
		return true;
	}

	// change to lowercase
	char tempname[kMaxInputLength];
	strcpy(tempname, newname);
	for (std::size_t i = 0; tempname[i]; i++) {
		tempname[i] = LOWER(tempname[i]);
	}

	// Does the desired name contain a string in the invalid list?
	for (std::size_t i = 0; i < num_invalid; i++) {
		if (strstr(tempname, invalid_list[i])) {
			return false;
		}
	}

	return true;
}

bool IsNameAvailable(char *newname) {
	return (IsValidName(newname) && IsNameOffline(newname));
}

bool IsNameOffline(char *newname) {
	for (auto dt = descriptor_list; dt; dt = dt->next) {
		if (dt->character && GET_NAME(dt->character) && !str_cmp(GET_NAME(dt->character), newname)) {
			return false;
		}
	}
	return true;
}

void ReadCharacterInvalidNamesList() {
	FILE *fp;
	char temp[256];

	if (!(fp = fopen(XNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" XNAME_FILE "' for reading");
		return;
	}

	num_invalid = 0;
	while (get_line(fp, temp) && num_invalid < kMaxInvalidNames)
		invalid_list[num_invalid++] = str_dup(temp);

	if (num_invalid >= kMaxInvalidNames) {
		log("SYSERR: Too many invalid names; change MAX_INVALID_NAMES in ban.c");
		exit(1);
	}

	fclose(fp);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
