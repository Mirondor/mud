#include "assist.h"

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "pk.h"
#include "fight_start.h"

void do_assist(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetEnemy()) {
		SendMsgToChar("Невозможно. Вы сражаетесь сами.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	CharData *helpee, *opponent;
	if (!*arg) {
		helpee = nullptr;
		for (const auto i : world[ch->in_room]->people) {
			if (i->GetEnemy() && i->GetEnemy() != ch
				&& ((ch->has_master() && ch->get_master() == i->get_master())
					|| ch->get_master() == i || i->get_master() == ch
					|| (AFF_FLAGGED(ch, EAffect::kCharmed) && ch->get_master() && ch->get_master()->get_master()
						&& (ch->get_master()->get_master() == i
							|| ch->get_master()->get_master() == i->get_master())))) {
				helpee = i;
				break;
			}
		}

		if (!helpee) {
			SendMsgToChar("Кому вы хотите помочь?\r\n", ch);
			return;
		}
	} else {
		if (!(helpee = get_char_vis(ch, arg, EFind::kCharInRoom))) {
			SendMsgToChar(NOPERSON, ch);
			return;
		}
	}
	if (helpee == ch) {
		SendMsgToChar("Вам могут помочь только Боги!\r\n", ch);
		return;
	}

	if (helpee->GetEnemy()) {
		opponent = helpee->GetEnemy();
	} else {
		opponent = nullptr;
		for (const auto i : world[ch->in_room]->people) {
			if (i->GetEnemy() == helpee) {
				opponent = i;
				break;
			}
		}
	}

	if (!opponent)
		act("Но никто не сражается с $N4!", false, ch, 0, helpee, kToChar);
	else if (!CAN_SEE(ch, opponent))
		act("Вы не видите противника $N1!", false, ch, 0, helpee, kToChar);
	else if (opponent == ch)
		act("Дык $E сражается с ВАМИ!", false, ch, 0, helpee, kToChar);
	else if (!may_kill_here(ch, opponent, NoArgument))
		return;
	else if (need_full_alias(ch, opponent))
		act("Используйте команду 'атаковать' для нападения на $N1.", false, ch, 0, opponent, kToChar);
	else if (set_hit(ch, opponent)) {
		act("Вы присоединились к битве, помогая $N2!", false, ch, 0, helpee, kToChar);
		act("$N решил$G помочь вам в битве!", 0, helpee, 0, ch, kToChar);
		act("$n вступил$g в бой на стороне $N1.", false, ch, 0, helpee, kToNotVict | kToArenaListen);
	}
}
