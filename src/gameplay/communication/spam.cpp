// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "spam.h"

#include "utils/logger.h"
#include "engine/entities/char_data.h"

using namespace antispam;

namespace {

// макс сообщений подряд от одного чара
const int MAX_MESSAGE_RUNNING = 10;
// х последних сообщений, обрабатываемых для проверок
const unsigned MESSAGE_LIST_SIZE = 31;
// макс сообщений от одного чара за последние MESSAGE_LIST_SIZE сообщений
const int MAX_MESSAGE_TOTAL = 15;
// кол-во учитыаемых в MAX_MESSAGE_TOTAL секунд между первой и текущей мессагой
// т.е. если даже чар сказал MAX_MESSAGE_TOTAL мессаг из последних MESSAGE_LIST_SIZE,
// но при этом между ними есть приличный временной зазор
const int MAX_TIME_TOTAL = 45;
// секунд между мессагами от одного чара (вместо тупо лага)
const int MIN_MESSAGE_TIME = 1;

enum { NORMAL_FLAG, RUNNING_FLAG, MIN_TIME_FLAG, TOTAL_FLAG };

typedef std::list<std::pair<long, time_t> > UidListType;
UidListType offtop_list;

void add_to_list(UidListType &list, long uid) {
	list.emplace_front(uid, time(nullptr));
	if (list.size() > MESSAGE_LIST_SIZE) {
		list.pop_back();
	}
}

int check_list(const UidListType & /*list*/, long uid) {
	int total = 0, running = 0;
	time_t first_message = time(nullptr);
	for (const auto &i : offtop_list) {
		if (uid != i.first) {
			running = -1;
		} else {
			if (running != -1) {
				++running;
				if (running >= MAX_MESSAGE_RUNNING) {
					return RUNNING_FLAG;
				}
			}
			if ((time(nullptr) - i.second) < MIN_MESSAGE_TIME) {
				return MIN_TIME_FLAG;
			}
			++total;
			first_message = i.second;
		}
	}
	if (total >= MAX_MESSAGE_TOTAL
		&& (time(nullptr) - first_message) < MAX_TIME_TOTAL) {
		return TOTAL_FLAG;
	}
	return NORMAL_FLAG;
}

int add_message(int mode, long uid) {
	switch (mode) {
		case antispam::kOfftopMode: {
			int flag = check_list(offtop_list, uid);
			if (NORMAL_FLAG == flag) {
				add_to_list(offtop_list, uid);
			} else {
				return flag;
			}
			break;
		}
		default: log("SYSERROR: мы не должны были сюда попасть (%s %s %d)", __FILE__, __func__, __LINE__);
			return NORMAL_FLAG;
	}

	return NORMAL_FLAG;
}

} // namespace

namespace antispam {

bool check(CharData *ch, int mode) {
	int flag = add_message(mode, GET_UID(ch));
	if (NORMAL_FLAG != flag) {
		std::stringstream text;
		text << "Спам-контроль: ";
		switch (flag) {
			case RUNNING_FLAG: text << "больше " << MAX_MESSAGE_RUNNING << " сообщений подряд";
				break;
			case MIN_TIME_FLAG: text << "интервал между сообщениями меньше " << MIN_MESSAGE_TIME << " секунды";
				break;
			case TOTAL_FLAG: text << "высокий процент ваших сообщений за последние " << MAX_TIME_TOTAL << " секунд";
				break;
			default: log("SYSERROR: мы не должны были сюда попасть (%s %s %d)", __FILE__, __func__, __LINE__);
				return true;
		}
		text << ".\r\n";
		SendMsgToChar(text.str(), ch);
		return false;
	}
	return true;
}

} // SpamSystem

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
