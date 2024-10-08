//#include "engine/ui/cmd/mercenary.h"

#include "engine/core/handler.h"
#include "engine/entities/char_player.h"
#include "engine/ui/modify.h"
#include "gameplay/statistics/mob_stat.h"
#include "gameplay/communication/talk.h"
#include "gameplay/ai/spec_procs.h"

#include <third_party_libs/fmt/include/fmt/format.h>

int do_social(CharData *ch, char *argument);

namespace MERC {
const int BASE_COST = 1000;
CharData *findMercboss(int room_rnum) {
	for (const auto tch : world[room_rnum]->people)
		if (GET_MOB_SPEC(tch) == mercenary)
			return tch;
	sprintf(buf1, "[ERROR] MERC::doList - вызвана команда, не найден моб, room %d", room_rnum);
	mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
	return nullptr;
};

void doList(CharData *ch, CharData *boss, bool isFavList) {
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	if (m->empty()) {
		if (CanUseFeat(ch, EFeat::kEmployer))
			tell_to_char(boss, ch, "Ступай, поначалу заведи знакомства, потом ко мне приходи.");
		else if (IS_SPELL_SET(ch, ESpell::kCharm, ESpellType::kKnow))
			tell_to_char(boss, ch, "Поищи себе марионетку, да потренируйся, а затем ко мне приходи.");
		else if (IS_IMMORTAL(ch))
			tell_to_char(boss, ch, "Не гневайся, боже, но не было у тебя последователей еще.");
		return;
	}
	if (IS_IMMORTAL(ch)) {
		sprintf(buf, "Вот, %s тварей земных, чьим разумом ты владел:",
				isFavList ? "краткий список" : "полный список");
	} else if (CanUseFeat(ch, EFeat::kEmployer)) {
		sprintf(buf, "%s тех, с кем знакомство ты водишь:",
				isFavList ? "Краткий список" : "Полный список");
	} else if (IS_SPELL_SET(ch, ESpell::kCharm, ESpellType::kKnow)) {
		sprintf(buf, "Вот %s тварей земных, чьим разумом ты владел с помощью чар колдовских:",
				isFavList ? "краткий список" : "полный список");
	}
	tell_to_char(boss, ch, buf);
	SendMsgToChar(ch,
				  " ##   Имя                                                   \r\n"
				  "------------------------------------------------------------\r\n");
	auto it = m->begin();
	std::stringstream out;
	std::string mobname;
	for (int num = 1; it != m->end(); ++it, ++num) {
		if (it->second.currRemortAvail == 0)
			continue;
		if (isFavList && it->second.isFavorite == 0)
			continue;
		mobname = mob_stat::PrintMobName(it->first, 54);
		out << fmt::format("{:<3})  {:<54.54}\r\n", num, mobname);
	}
	page_string(ch->desc, out.str());
	SendMsgToChar(ch, "------------------------------------------------------------\r\n");
	sprintf(buf,
			"А всего за %d кун моя ватага приведёт тебе любого из них живым и невредимым.",
			1000 * (GetRealRemort(ch) + 1));
	tell_to_char(boss, ch, buf);
	snprintf(buf, kMaxInputLength, "ухмы %s", GET_NAME(ch));
	do_social(boss, buf);
};

void doStat(CharData *ch) {
	if (!ch) return;
};

void doBring(CharData *ch, CharData *boss, unsigned int pos, char *bank) {
	CharData *mob;
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	const int cost = MERC::BASE_COST * (GetRealRemort(ch) + 1);
	MobRnum rnum;
	//std::map<int, MERCDATA>::iterator
	auto it = m->begin();
	for (unsigned int num = 1; it != m->end(); ++it, ++num) {
		if (num != pos)
			continue;
		if (it->second.currRemortAvail == 0) {
			sprintf(buf, "Протри глаза, такой персонаж тебе неведом!");
			tell_to_char(boss, ch, buf);
			return;
		}

		if ((!isname(bank, "банк bank") && cost > ch->get_gold()) ||
			(isname(bank, "банк bank") && cost > ch->get_bank())) {
			sprintf(buf, "Мои услуги стоят %d %s - это тебе не по карману.", cost,
					GetDeclensionInNumber(cost, EWhat::kMoneyU));
			tell_to_char(boss, ch, buf);
			return;
		}

		if ((rnum = GetMobRnum(it->first)) < 0) {
			sprintf(buf, "С персонажем стряслось что то, не могу его найти.");
			tell_to_char(boss, ch, buf);
			sprintf(buf1, "[ERROR] MERC::doBring, не найден моб, vnum: %d", it->first);
			mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
			return;
		}
		mob = ReadMobile(rnum, kReal);
		PlaceCharToRoom(mob, ch->in_room);
		if (IS_SPELL_SET(ch, ESpell::kCharm, ESpellType::kKnow)) {
			act("$n окрикнул$g своих парней и скрыл$u из виду.",
				true, boss, nullptr, nullptr, kToRoom);
			act("Спустя некоторое время, взмыленная ватага вернулась, волоча на аркане $n3.",
				true, mob, nullptr, nullptr, kToRoom);
		} else {
			act("$n вскочил$g и скрыл$u из виду.",
				true, boss, nullptr, nullptr, kToRoom);
			sprintf(buf, "Спустя некоторое время, %s вернул$U, ведя за собой $n3.", boss->get_npc_name().c_str());
			act(buf, true, mob, nullptr, ch, kToRoom);
		}
		if (!IS_IMMORTAL(ch)) {
			if (isname(bank, "банк bank"))
				ch->remove_bank(cost);
			else
				ch->remove_gold(cost);
		}
		if (NPC_FLAGGED(mob, ENpcFlag::kNoMercList)) {
			if (GET_SEX(mob) == EGender::kMale)
				SendMsgToChar(ch, "%s показал большую дулю и свалил от вас подальше.\r\n", mob->get_npc_name().c_str());
			else
				SendMsgToChar(ch, "%s показала большую дулю и свалила от вас подальше.\r\n", mob->get_npc_name().c_str());
			ExtractCharFromWorld(mob, false);
		}
	}
};

void doForget(CharData *ch, CharData *boss, unsigned int pos) {
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	auto it = m->begin();
	for (unsigned int num = 1; it != m->end(); ++it, ++num) {
		if (num != pos)
			continue;
		if (it->second.currRemortAvail == 0) {
			sprintf(buf, "Протри глаза, такой персонаж тебе неведом!");
			tell_to_char(boss, ch, buf);
			return;
		}
		it->second.isFavorite = !it->second.isFavorite;
		if (it->second.isFavorite)
			sprintf(buf, "Персонаж %s добавлен в список любимчиков.", mob_stat::PrintMobName(it->first, 54).c_str());
		else
			sprintf(buf, "Персонаж %s попал в опалу.", mob_stat::PrintMobName(it->first, 54).c_str());
		tell_to_char(boss, ch, buf);
		return;
	}
	sprintf(buf1, "[ERROR] MERC::doForget, не найден моб, pos: %d", pos);
	mudlog(buf1, LogMode::CMP, 1, EOutputStream::SYSLOG, 1);
};

unsigned int getPos(char *arg, CharData *ch, CharData *boss) {
	unsigned int pos = 0;
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	try {
		pos = std::stoi(arg);
	} catch (const std::invalid_argument &) {
		pos = 0;
	}
	if (pos < 1 || pos > m->size() || m->empty()) {
		sprintf(buf, "Протри глаза, такой персонаж тебе неведом!");
		tell_to_char(boss, ch, buf);
		return 0;
	}
	return pos;
}

}

int mercenary(CharData *ch, void * /*me*/, int cmd, char *argument) {
	if (!ch || !ch->desc || ch->IsNpc())
		return 0;
	if (!(CMD_IS("наемник") || CMD_IS("mercenary")))
		return 0;

	CharData *boss = MERC::findMercboss(ch->in_room);
	if (!boss) return 0;

	if (!IS_IMMORTAL(ch) && !CanUseFeat(ch, EFeat::kEmployer) && ch->GetClass() != ECharClass::kCharmer) {
		tell_to_char(boss, ch, "Эти знания тебе недоступны, ступай с миром");
		return 0;
	}

	char subCmd[kMaxInputLength];
	char cmdParam[kMaxInputLength];
	char bank[kMaxInputLength];
	unsigned int pos;

	three_arguments(argument, subCmd, cmdParam, bank);
	if (utils::IsAbbr(subCmd, "стат") || utils::IsAbbr(subCmd, "stat")) {
		return (1);
	} else if (utils::IsAbbr(subCmd, "список") || utils::IsAbbr(subCmd, "list")) {
		if (utils::IsAbbr(cmdParam, "полный") || utils::IsAbbr(cmdParam, "full"))
			MERC::doList(ch, boss, false);
		else
			MERC::doList(ch, boss, true);
		return (1);
	} else if (utils::IsAbbr(subCmd, "привести") || utils::IsAbbr(subCmd, "bring")) {
		pos = MERC::getPos(cmdParam, ch, boss);
		if (pos == 0) return (1);
		MERC::doBring(ch, boss, pos, bank);
		return (1);
	} else if (utils::IsAbbr(subCmd, "фаворит") || utils::IsAbbr(subCmd, "favorite")) {
		pos = MERC::getPos(cmdParam, ch, boss);
		if (pos == 0) return (1);
		MERC::doForget(ch, boss, pos);
		return (1);
	} else
		tell_to_char(boss, ch, "Доступные команды: список (полный), привести <номер> (банк), фаворит <номер>");
	return (1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

