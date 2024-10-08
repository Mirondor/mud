// Copyright (c) 2012 Krodo
// Part of Bylins http://www.mud.ru

#include "obj_enchant.h"
#include "engine/entities/obj_data.h"
#include "engine/ui/color.h"
#include "engine/entities/char_data.h"

namespace ObjectEnchant {

enchant::enchant()
	: type_(-1), weight_(0), ndice_(0), sdice_(0) {
	affects_flags_ = clear_flags;
	extra_flags_ = clear_flags;
	no_flags_ = clear_flags;
}

enchant::enchant(ObjData *obj) {
	name_ = !GET_OBJ_PNAME(obj, 4).empty() ? GET_OBJ_PNAME(obj, 4).c_str() : "<null>";
	type_ = ENCHANT_FROM_OBJ;

	for (int i = 0; i < kMaxObjAffect; i++) {
		if (obj->get_affected(i).location != EApply::kNone
			&& obj->get_affected(i).modifier != 0) {
			affected_.push_back(obj->get_affected(i));
		}
	}

	affects_flags_ = GET_OBJ_AFFECTS(obj);
	extra_flags_ = GET_OBJ_EXTRA(obj);
	extra_flags_.unset(EObjFlag::kTicktimer);
	no_flags_ = GET_OBJ_NO(obj);
	weight_ = GET_OBJ_VAL(obj, 0);
	ndice_ = 0;
	sdice_ = 0;
}

void enchant::print(CharData *ch) const {
	SendMsgToChar(ch, "Зачаровано %s :%s\r\n", name_.c_str(), kColorCyn);

	for (std::vector<obj_affected_type>::const_iterator i = affected_.begin(),
			 iend = affected_.end(); i != iend; ++i) {
		print_obj_affects(ch, *i);
	}

	if (affects_flags_.sprintbits(weapon_affects, buf2, ",")) {
		SendMsgToChar(ch, "%s   аффекты: %s%s\r\n",
					  kColorCyn, buf2, kColorNrm);
	}

	if (extra_flags_.sprintbits(extra_bits, buf2, ",")) {
		SendMsgToChar(ch, "%s   экстрафлаги: %s%s\r\n",
					  kColorCyn, buf2, kColorNrm);
	}

	if (no_flags_.sprintbits(no_bits, buf2, ",")) {
		SendMsgToChar(ch, "%s   неудобен: %s%s\r\n",
					  kColorCyn, buf2, kColorNrm);
	}

	if (weight_ != 0) {
		SendMsgToChar(ch, "%s   %s вес на %d%s\r\n", kColorCyn,
					  weight_ > 0 ? "увеличивает" : "уменьшает",
					  abs(weight_), kColorNrm);
	}

	if (ndice_ != 0 || sdice_ != 0) {
		if (ndice_ >= 0 && sdice_ >= 0) {
			SendMsgToChar(ch, "%s   увеличивает урон на %dD%d%s\r\n",
						  kColorCyn, abs(ndice_), abs(sdice_), kColorNrm);
		} else if (ndice_ <= 0 && sdice_ <= 0) {
			SendMsgToChar(ch, "%s   уменьшает урон на %dD%d%s\r\n",
						  kColorCyn, abs(ndice_), abs(sdice_), kColorNrm);
		} else {
			SendMsgToChar(ch, "%s   изменяет урон на %+dD%+d%s\r\n",
						  kColorCyn, ndice_, sdice_, kColorNrm);
		}
	}
}

std::string enchant::print_to_file() const {
	std::stringstream out;
	out << "Ench: I " << name_ << "\n" << " T " << type_ << "\n";

	for (std::vector<obj_affected_type>::const_iterator i = affected_.begin(),
			 iend = affected_.end(); i != iend; ++i) {
		out << " A " << i->location << " " << i->modifier << "\n";
	}

	*buf = '\0';
	affects_flags_.tascii(FlagData::kPlanesNumber, buf);
	out << " F " << buf << "\n";

	*buf = '\0';
	extra_flags_.tascii(FlagData::kPlanesNumber, buf);
	out << " E " << buf << "\n";

	*buf = '\0';
	no_flags_.tascii(FlagData::kPlanesNumber, buf);
	out << " N " << buf << "\n";

	out << " W " << weight_ << "\n";
	out << " B " << ndice_ << "\n";
	out << " C " << sdice_ << "\n";
	out << "~\n";

	return out.str();
}

void correct_values(ObjData *obj) {
	obj->set_weight(std::max(1, GET_OBJ_WEIGHT(obj)));
	obj->set_val(1, std::max(0, GET_OBJ_VAL(obj, 1)));
	obj->set_val(2, std::max(0, GET_OBJ_VAL(obj, 2)));
}

void enchant::apply_to_obj(ObjData *obj) const {
	for (auto i = affected_.cbegin(), iend = affected_.cend(); i != iend; ++i) {
		for (int k = 0; k < kMaxObjAffect; k++) {
			if (obj->get_affected(k).location == i->location) {
				obj->add_affected(k, i->modifier);
				break;
			} else if (obj->get_affected(k).location == EApply::kNone) {
				obj->set_affected(k, *i);
				break;
			}
		}
	}

	obj->add_affect_flags(affects_flags_);
	obj->add_extra_flags(extra_flags_);
	obj->add_no_flags(no_flags_);
	obj->add_weight(weight_);

	if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
		obj->add_val(1, ndice_);
		obj->add_val(2, sdice_);
	}

	correct_values(obj);
	obj->add_enchant(*this);
}

void Enchants::add(const enchant &ench) {
	list_.push_back(ench);
}

bool Enchants::check(int type) const {
	for (auto i = list_.cbegin(), iend = list_.cend(); i != iend; ++i) {
		if (i->type_ == type) {
			return true;
		}
	}
	return false;
}

std::string Enchants::print_to_file() const {
	std::string out;
	for (auto i = list_.cbegin(), iend = list_.cend(); i != iend; ++i) {
		out += i->print_to_file();
	}
	return out;
}

void Enchants::print(CharData *ch) const {
	for (auto i = list_.cbegin(), iend = list_.cend(); i != iend; ++i) {
		i->print(ch);
	}
}

bool Enchants::empty() const {
	return list_.empty();
}

void Enchants::update_set_bonus(ObjData *obj, const obj_sets::ench_type &set_ench) {
	for (auto i = list_.begin(); i != list_.end(); ++i) {
		if (i->type_ == ENCHANT_FROM_SET) {
			if (i->weight_ != set_ench.weight
				|| i->ndice_ != set_ench.ndice
				|| i->sdice_ != set_ench.sdice) {
				// вес
				obj->add_weight(set_ench.weight - i->weight_);
				// дайсы пушек
				if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
					obj->add_val(1, set_ench.ndice - i->ndice_);
					obj->add_val(2, set_ench.sdice - i->sdice_);
				}
				correct_values(obj);
				i->weight_ = set_ench.weight;
				i->ndice_ = set_ench.ndice;
				i->sdice_ = set_ench.sdice;
			}
			return;
		}
	}

	ObjectEnchant::enchant tmp;
	tmp.type_ = ObjectEnchant::ENCHANT_FROM_SET;
	tmp.name_ = "набором предметов";
	tmp.weight_ = set_ench.weight;
	tmp.ndice_ = set_ench.ndice;
	tmp.sdice_ = set_ench.sdice;
	tmp.apply_to_obj(obj);
}

void Enchants::remove_set_bonus(ObjData *obj) {
	for (auto i = list_.begin(); i != list_.end(); ++i) {
		if (i->type_ == ENCHANT_FROM_SET) {
			obj->sub_weight(i->weight_);
			obj->sub_val(1, i->ndice_);
			obj->sub_val(2, i->sdice_);
			correct_values(obj);
			list_.erase(i);
			return;
		}
	}
}

} // obj

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
