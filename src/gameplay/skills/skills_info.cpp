#include "engine/ui/color.h"
#include "engine/db/global_objects.h"

struct AttackMessages fight_messages[kMaxMessages];

using DataNode = parser_wrapper::DataNode;
using ItemPtr = SkillInfoBuilder::ItemPtr;

void SkillsLoader::Load(DataNode data) {
	MUD::Skills().Init(data.Children());
}

void SkillsLoader::Reload(DataNode data) {
	MUD::Skills().Reload(data.Children());
}

ItemPtr SkillInfoBuilder::Build(DataNode &node) {
	auto skill_info = ParseObligatoryValues(node);
	if (skill_info) {
		ParseDispensableValues(skill_info, node);
	}
	return skill_info;
}

void SkillInfoBuilder::ParseDispensableValues(ItemPtr &item_ptr, DataNode &node) {
	try {
		item_ptr->name = parse::ReadAsStr(node.GetValue("name"));
		item_ptr->short_name = parse::ReadAsStr(node.GetValue("abbr"));
		item_ptr->save_type = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
		item_ptr->difficulty = parse::ReadAsInt(node.GetValue("difficulty"));
		item_ptr->cap = parse::ReadAsInt(node.GetValue("cap"));
	} catch (std::exception &e) {
		err_log("invalid skill description (incorrect value: %s). Setted by default;", e.what());
	}
}

ItemPtr SkillInfoBuilder::ParseObligatoryValues(DataNode &node) {
	auto id{ESkill::kUndefined};
	auto mode{EItemMode::kDisabled};
	try {
		id = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
		mode = SkillInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
	} catch (std::exception &e) {
		err_log("incorrect skill id (%s). ", e.what());
		return nullptr;
	}

	return std::make_shared<SkillInfo>(id, mode);
}

void SkillInfo::Print(std::ostringstream &buffer) const {
	buffer << "Print skill:" << "\r\n"
		   << " Id: " << kColorGrn << NAME_BY_ITEM<ESkill>(GetId()) << kColorNrm << "\r\n"
		   << " Name: " << kColorGrn << name << kColorNrm << "\r\n"
		   << " Abbreviation: " << kColorGrn << short_name << kColorNrm << "\r\n"
		   << " Save type: " << kColorGrn << NAME_BY_ITEM<ESaving>(save_type) << kColorNrm << "\r\n"
		   << " Difficulty: " << kColorGrn << difficulty << kColorNrm << "\r\n"
		   << " Skill cap: " << kColorGrn << cap << kColorNrm << "\r\n"
		   << " Mode: " << kColorGrn << NAME_BY_ITEM<EItemMode>(GetMode()) << kColorNrm << "\r\n";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
