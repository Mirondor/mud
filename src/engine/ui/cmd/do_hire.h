#ifndef BYLINS_HIRE_H
#define BYLINS_HIRE_H

#include "gameplay/magic/spells_constants.h"

class CharData;

void do_findhelpee(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_freehelpee(CharData *ch, char * /* argument*/, int/* cmd*/, int/* subcmd*/);
int GetReformedCharmiceHp(CharData *ch, CharData *victim, ESpell spell_id);

#endif //BYLINS_HIRE_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

