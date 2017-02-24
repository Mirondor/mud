/* ************************************************************************
*  File: dg_db_scripts.cpp                                 Part of Bylins *
*                                                                         *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                         *
*  $Date$                                           *
*  $Revision$                                                   *
************************************************************************ */
#include "dg_db_scripts.hpp"

#include "obj.hpp"
#include "dg_scripts.h"
#include "db.h"
#include "handler.h"
#include "dg_event.h"
#include "comm.h"
#include "spells.h"
#include "skills.h"
#include "im.h"
#include "features.hpp"
#include "char.hpp"
#include "interpreter.h"
#include "room.hpp"
#include "magic.h"
#include "boards.h"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "temp_spells.hpp"

#include <algorithm>
#include <stack>

void trig_data_free(TRIG_DATA * this_data);
//����_�������� : [����_��������_�������_���������_������ ������ : [������������ � ���� ������������ (����� ��������/�����/������)]]
trigger_to_owners_map_t owner_trig;

extern INDEX_DATA **trig_index;
extern int top_of_trigt;

extern INDEX_DATA *mob_index;

int check_recipe_values(CHAR_DATA * ch, int spellnum, int spelltype, int showrecipe);

// TODO: Get rid of me
char* dirty_indent_trigger(char* cmd , int* level)
{
	static std::stack<std::string> indent_stack;

	*level = std::max(0, *level);
	if (*level == 0)
	{
		std::stack<std::string> empty_stack;
		indent_stack.swap(empty_stack);
	}

	// ������ ��������
	int currlev, nextlev;
	currlev = nextlev = *level;

	if (!cmd)
	{
		return cmd;
	}

	// ������� ������� ������ �������.
	char* ptr = cmd;
	skip_spaces(&ptr);

	// ptr �������� ������ ��� ������ ��������.
	if (!strn_cmp("case ", ptr , 5) || !strn_cmp("default", ptr , 7))
	{
		// ���������������� case (��� default ����� case) ��� break
		if (!indent_stack.empty()
			&& !strn_cmp("case ", indent_stack.top().c_str(), 5))
		{
			--currlev;
		}
		else
		{
			indent_stack.push(ptr);
		}
		nextlev = currlev + 1;
	}
	else if (!strn_cmp("if ", ptr , 3) || !strn_cmp("while ", ptr , 6)
		|| !strn_cmp("foreach ", ptr, 8) || !strn_cmp("switch ", ptr, 7))
	{
		++nextlev;
		indent_stack.push(ptr);
	}
	else if (!strn_cmp("elseif ", ptr, 7) || !strn_cmp("else", ptr, 4))
	{
		--currlev;
	}
	else if (!strn_cmp("break", ptr, 5) || !strn_cmp("end", ptr, 3)
		|| !strn_cmp("done", ptr, 4))
	{
		// � switch ����������� break ����� �������� � ����� ������ done|end
		if ((!strn_cmp("done", ptr, 4) || !strn_cmp("end", ptr, 3))
			&& !indent_stack.empty()
			&& (!strn_cmp("case ", indent_stack.top().c_str(), 5)
				|| !strn_cmp("default", indent_stack.top().c_str() , 7)))
		{
			--currlev;
			--nextlev;
			indent_stack.pop();
		}
		if (!indent_stack.empty())
		{
			indent_stack.pop();
		}
		--nextlev;
		--currlev;
	}

	if (nextlev < 0) nextlev = 0;
	if (currlev < 0) currlev = 0;

	// ��������� �������������� �������

	char* tmp = (char *) malloc(currlev * 2 + 1);
	memset(tmp, 0x20, currlev*2);
	tmp[currlev*2] = '\0';

	tmp = str_add(tmp, ptr);

	cmd = (char *)realloc(cmd, strlen(tmp) + 1);
	cmd = strcpy(cmd, tmp);

	free(tmp);

	*level = nextlev;
	return cmd;
}

void indent_trigger(std::string& cmd, int* level)
{
	char* cmd_copy = str_dup(cmd.c_str());;
	cmd_copy = dirty_indent_trigger(cmd_copy, level);
	cmd = cmd_copy;
}

/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
TRIG_DATA *read_trigger(int nr)
{
	index_data *index;
	if (nr >= top_of_trigt || nr == -1)
	{
		return NULL;
	}

	if ((index = trig_index[nr]) == NULL)
	{
		return NULL;
	}

	TRIG_DATA *trig = new TRIG_DATA(*index->proto);
	index->number++;

	return trig;
}

// release memory allocated for a variable list
void free_varlist(struct trig_var_data *vd)
{
	struct trig_var_data *i, *j;

	for (i = vd; i;)
	{
		j = i;
		i = i->next;
		if (j->name)
			free(j->name);
		if (j->value)
			free(j->value);
		free(j);
	}
}

// release memory allocated for a script
void free_script(SCRIPT_DATA* sc)
{
	if (sc == NULL)
		return;

	extract_script(sc);

	free_varlist(sc->global_vars);

	free(sc);
}

void trig_data_free(TRIG_DATA * this_data)
{
	free_varlist(this_data->var_list);

	if (this_data->wait_event)
	{
		// �� ����� �������� ����������� callback ����� trig_wait_event
		// � ������ ��������� ������������ ��������� info
		free(this_data->wait_event->info);
		remove_event(this_data->wait_event);
	}

	delete this_data;
}

// vnum_owner - ����, ������� ���������� ������ ����
// vnum_trig - ���� ������������� �����
// vnum - � ���� ����������� ����
void add_trig_to_owner(int vnum_owner, int vnum_trig, int vnum)
{
	if (owner_trig[vnum_trig].find(vnum_owner) != owner_trig[vnum_trig].end())
	{
		const auto& triggers_set = owner_trig[vnum_trig][vnum_owner];
		const bool flag_trig = triggers_set.find(vnum) != triggers_set.end();

		if (!flag_trig)
		{
			owner_trig[vnum_trig][vnum_owner].insert(vnum);
		}
	}
	else
	{
		triggers_set_t tmp_vector = { vnum };
		owner_trig[vnum_trig].emplace(-1, tmp_vector);
	}
}

void dg_obj_trigger(char *line, OBJ_DATA * obj)
{
	char junk[8];
	int vnum, rnum, count;

	count = sscanf(line, "%s %d", junk, &vnum);

	if (count != 2)  	// should do a better job of making this message
	{
		log("SYSERR: Error assigning trigger!");
		return;
	}

	rnum = real_trigger(vnum);
	if (rnum < 0)
	{
		sprintf(line, "SYSERR: Trigger vnum #%d asked for but non-existant!", vnum);
		log("%s",line);
		return;
	}

	// ��� ������ ����������, ���� �� ����� ���� � ��� � ����������
	if (owner_trig.find(vnum) == owner_trig.end())
	{
		owner_to_triggers_map_t tmp_map;
		owner_trig.emplace(vnum, tmp_map);	
	}
	add_trig_to_owner(-1, vnum, GET_OBJ_VNUM(obj));

	
	obj->add_proto_script(vnum);
}

extern CHAR_DATA *mob_proto;

void assign_triggers(void *i, int type)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	ROOM_DATA *room;
	int rnum;
	char buf[256];

	switch (type)
	{
	case MOB_TRIGGER:
		mob = (CHAR_DATA *) i;
		for (const auto trigger_vnum : *mob_proto[GET_MOB_RNUM(mob)].proto_script)
		{
			rnum = real_trigger(trigger_vnum);
			if (rnum == -1)
			{
				sprintf(buf, "SYSERR: trigger #%d non-existent, for mob #%d",
					trigger_vnum, mob_index[mob->nr].vnum);
				log("%s",buf);
			}
			else
			{
				if (trig_index[rnum]->proto->get_attach_type() != MOB_TRIGGER)
				{
					sprintf(buf, "SYSERR: trigger #%d has wrong attach_type: %d, for mob #%d",
						trigger_vnum, static_cast<int>(trig_index[rnum]->proto->get_attach_type()),
						mob_index[mob->nr].vnum);
					mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
				}
				else
				{
					if (!SCRIPT(mob))
					{
						CREATE(mob->script, 1);
					}
					add_trigger(SCRIPT(mob), read_trigger(rnum), -1);


					if (owner_trig.find(trigger_vnum) == owner_trig.end())
					{
						owner_to_triggers_map_t tmp_map;
						owner_trig.emplace(trigger_vnum, tmp_map);
					}
					add_trig_to_owner(-1, trigger_vnum, GET_MOB_VNUM(mob));
				}
			}
		}
		break;

	case OBJ_TRIGGER:
		obj = (OBJ_DATA *) i;
		for (const auto trigger_vnum : obj_proto.proto_script(GET_OBJ_RNUM(obj)))
		{
			rnum = real_trigger(trigger_vnum);
			if (rnum == -1)
			{
				sprintf(buf, "SYSERR: trigger #%d non-existent, for obj #%d",
					trigger_vnum, obj->get_vnum());
				log("%s",buf);
			}
			else
			{
				if (trig_index[rnum]->proto->get_attach_type() != OBJ_TRIGGER)
				{
					sprintf(buf, "SYSERR: trigger #%d has wrong attach_type: %d, for obj #%d",
						trigger_vnum,
						static_cast<int>(trig_index[rnum]->proto->get_attach_type()),
						obj->get_vnum());
					mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
				}
				else
				{
					if (!obj->get_script())
					{
						obj->set_script(new SCRIPT_DATA());
					}
					add_trigger(obj->get_script().get(), read_trigger(rnum), -1);
					if (owner_trig.find(trigger_vnum) == owner_trig.end())
					{
						owner_to_triggers_map_t tmp_map;
						owner_trig.emplace(trigger_vnum, tmp_map);
					}
					add_trig_to_owner(-1, trigger_vnum, GET_OBJ_VNUM(obj));
				}
			}
		}
		break;

	case WLD_TRIGGER:
		room = (ROOM_DATA *) i;
		for (const auto trigger_vnum : *room->proto_script)
		{
			rnum = real_trigger(trigger_vnum);
			if (rnum == -1)
			{
				sprintf(buf, "SYSERR: trigger #%d non-existant, for room #%d",
					trigger_vnum, room->number);
				log("%s",buf);
			}
			else
			{
				if (trig_index[rnum]->proto->get_attach_type() != WLD_TRIGGER)
				{
					sprintf(buf, "SYSERR: trigger #%d has wrong attach_type: %d, for room #%d",
						trigger_vnum, static_cast<int>(trig_index[rnum]->proto->get_attach_type()),
						room->number);
					mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
				}
				else
				{
					if (!SCRIPT(room))
					{
						room->script = new SCRIPT_DATA();
					}
					add_trigger(SCRIPT(room), read_trigger(rnum), -1);
					if (owner_trig.find(trigger_vnum) == owner_trig.end())
					{
						owner_to_triggers_map_t tmp_map;
						owner_trig.emplace(trigger_vnum, tmp_map);
					}
					add_trig_to_owner(-1, trigger_vnum, room->number);
				}
			}
		}
		break;

	default:
		log("SYSERR: unknown type for assign_triggers()");
		break;
	}
}

void trg_featturn(CHAR_DATA * ch, int featnum, int featdiff)
{
	if (HAVE_FEAT(ch, featnum))
	{
		if (featdiff)
			return;
		else
		{
			sprintf(buf, "�� �������� ����������� '%s'.\r\n", feat_info[featnum].name);
			send_to_char(buf, ch);
			UNSET_FEAT(ch, featnum);
		}
	}
	else
	{
		if (featdiff)
		{
			sprintf(buf, "�� ������ ����������� '%s'.\r\n", feat_info[featnum].name);
			send_to_char(buf, ch);
			if (feat_info[featnum].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
				SET_FEAT(ch, featnum);
		};
	}
}

void trg_skillturn(CHAR_DATA * ch, const ESkill skillnum, int skilldiff, int vnum)
{
	const int ch_kin = static_cast<int>(GET_KIN(ch));
	const int ch_class = static_cast<int>(GET_CLASS(ch));

	if (ch->get_trained_skill(skillnum))
	{
		if (skilldiff)
		{
			return;
		}

		ch->set_skill(skillnum, 0);
		send_to_char(ch, "��� ������ ������ '%s'.\r\n", skill_name(skillnum));
		log("Remove %s from %s (trigskillturn)", skill_name(skillnum), GET_NAME(ch));
	}
	else if (skilldiff
		&& skill_info[skillnum].classknow[ch_class][ch_kin] == KNOW_SKILL)
	{
		ch->set_skill(skillnum, 5);
		send_to_char(ch, "�� ������� ������ '%s'.\r\n", skill_name(skillnum));
		log("Add %s to %s (trigskillturn)trigvnum %d", skill_name(skillnum), GET_NAME(ch), vnum);
	}
}

void trg_skilladd(CHAR_DATA * ch, const ESkill skillnum, int skilldiff, int vnum)
{
	int skill = ch->get_trained_skill(skillnum);
	ch->set_skill(skillnum, (MAX(1, MIN(ch->get_trained_skill(skillnum) + skilldiff, 200))));

	if (skill > ch->get_trained_skill(skillnum))
	{
		send_to_char(ch, "���� ������ '%s' ����������.\r\n", skill_name(skillnum));
		log("Decrease %s to %s from %d to %d (diff %d)(trigskilladd) trigvnum %d", skill_name(skillnum), GET_NAME(ch), skill, ch->get_trained_skill(skillnum), skilldiff, vnum);
	}
	else if (skill < ch->get_trained_skill(skillnum))
	{
		send_to_char(ch, "�� �������� ���� ������ '%s'.\r\n", skill_name(skillnum));
		log("Raise %s to %s from %d to %d (diff %d)(trigskilladd) trigvnum %d", skill_name(skillnum), GET_NAME(ch), skill, ch->get_trained_skill(skillnum), skilldiff, vnum);
	}
	else
	{
		send_to_char(ch, "���� ������ �������� ���������� '%s '.\r\n", skill_name(skillnum));
		log("Unchanged %s on %s (trigskilladd) trigvnum %d", skill_name(skillnum), GET_NAME(ch), vnum);
	}
}

void trg_spellturn(CHAR_DATA * ch, int spellnum, int spelldiff, int vnum)
{
	int spell = GET_SPELL_TYPE(ch, spellnum);
	if (spell & SPELL_KNOW)
	{
		if (spelldiff) return;

		REMOVE_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW);
		if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP))
			GET_SPELL_MEM(ch, spellnum) = 0;
		send_to_char(ch, "�� ������� ������ ���������� '%s'.\r\n", spell_name(spellnum));
		log("Remove %s from %s (trigspell) trigvnum %d", spell_name(spellnum), GET_NAME(ch), vnum);
	}
	else if (spelldiff)
	{
		SET_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW);
		send_to_char(ch, "�� �������� ���������� '%s'.\r\n", spell_name(spellnum));
		log("Add %s to %s (trigspell) trigvnum %d", spell_name(spellnum), GET_NAME(ch), vnum);
	}
}

void trg_spellturntemp(CHAR_DATA * ch, int spellnum, int spelldiff, int vnum)
{
	if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW))
	{
		Temporary_Spells::add_spell(ch, spellnum, time(0), spelldiff);
		send_to_char(ch, "�� �������� ���������� '%s'.\r\n", spell_name(spellnum));
		log("Remove %s from %s (trigspell) trigvnum %d", spell_name(spellnum), GET_NAME(ch), vnum);
	}
}

void trg_spelladd(CHAR_DATA * ch, int spellnum, int spelldiff, int vnum)
{
	int spell = GET_SPELL_MEM(ch, spellnum);
	GET_SPELL_MEM(ch, spellnum) = MAX(0, MIN(spell + spelldiff, 50));

	if (spell > GET_SPELL_MEM(ch, spellnum))
	{
		if (GET_SPELL_MEM(ch, spellnum))
		{
			log("Remove custom spell %s to %s (trigspell) trigvnum %d", spell_name(spellnum), GET_NAME(ch), vnum);
			sprintf(buf, "�� ������ ����� ���������� '%s'.\r\n", spell_name(spellnum));
		}
		else
		{
			sprintf(buf, "�� ������ ��� ���������� '%s'.\r\n", spell_name(spellnum));
			//REMOVE_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP);
			log("Remove all spells %s to %s (trigspell) trigvnum %d", spell_name(spellnum), GET_NAME(ch), vnum);
		}
		send_to_char(buf, ch);
	}
	else if (spell < GET_SPELL_MEM(ch, spellnum))
	{
		/*if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW))
			SET_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP);*/
		send_to_char(ch, "�� ������� ��������� ���������� '%s'.\r\n", spell_name(spellnum));
		log("Add %s to %s (trigspell) trigvnum %d", spell_name(spellnum), GET_NAME(ch), vnum);
	}
}

void trg_spellitem(CHAR_DATA * ch, int spellnum, int spelldiff, int spell)
{
	char type[MAX_STRING_LENGTH];

	if ((spelldiff && IS_SET(GET_SPELL_TYPE(ch, spellnum), spell)) ||
			(!spelldiff && !IS_SET(GET_SPELL_TYPE(ch, spellnum), spell)))
		return;
	if (!spelldiff)
	{
		REMOVE_BIT(GET_SPELL_TYPE(ch, spellnum), spell);
		switch (spell)
		{
		case SPELL_SCROLL:
			strcpy(type, "�������� ������");
			break;
		case SPELL_POTION:
			strcpy(type, "������������� �������");
			break;
		case SPELL_WAND:
			strcpy(type, "������������ ������");
			break;
		case SPELL_ITEMS:
			strcpy(type, "���������� �����");
			break;
		case SPELL_RUNES:
			strcpy(type, "������������� ���");
			break;
		};
		sprintf(buf, "�� �������� ������ %s '%s'", type, spell_name(spellnum));
	}
	else
	{
		SET_BIT(GET_SPELL_TYPE(ch, spellnum), spell);
		switch (spell)
		{
		case SPELL_SCROLL:
			if (!ch->get_skill(SKILL_CREATE_SCROLL))
				ch->set_skill(SKILL_CREATE_SCROLL, 5);
			strcpy(type, "�������� ������");
			break;
		case SPELL_POTION:
			if (!ch->get_skill(SKILL_CREATE_POTION))
				ch->set_skill(SKILL_CREATE_POTION, 5);
			strcpy(type, "������������� �������");
			break;
		case SPELL_WAND:
			if (!ch->get_skill(SKILL_CREATE_WAND))
				ch->set_skill(SKILL_CREATE_WAND, 5);
			strcpy(type, "������������ ������");
			break;
		case SPELL_ITEMS:
			strcpy(type, "���������� �����");
			break;
		case SPELL_RUNES:
			strcpy(type, "������������� ���");
			break;
		};
		sprintf(buf, "�� ��������� ������ %s '%s'", type, spell_name(spellnum));
		send_to_char(buf, ch);
		check_recipe_items(ch, spellnum, spell, TRUE);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
