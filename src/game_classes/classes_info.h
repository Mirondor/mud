/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных классам скиллах и спеллах должна лежать здесь.
*/

#ifndef BYLINS_SRC_CLASSES_CLASSES_INFO_H_
#define BYLINS_SRC_CLASSES_CLASSES_INFO_H_

#include "boot/cfg_manager.h"
#include "game_classes/classes_constants.h"
#include "skills.h"
#include "structs/info_container.h"

#include <optional>

namespace classes {

class ClassesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

struct ClassSkillInfo {
	using Ptr = std::unique_ptr<ClassSkillInfo>;

	ESkill id{ESkill::kIncorrect};
	int min_level{kLevelImplementator};
	int min_remort{kMaxRemort + 1};
	long improve{kMinImprove};
};

struct CharClassInfo : public info_container::IItem<ECharClass>{
	CharClassInfo() {
		skills = std::make_unique<Skills>();
	}

	/* Базовые поля */
	ECharClass id{ECharClass::kFirst};
	EItemMode mode{EItemMode::kEnabled};

	[[nodiscard]] ECharClass GetId() const final { return id; };
	[[nodiscard]] EItemMode GetMode() const final { return mode; };

	/* Умения класса */
	using Skills = std::unordered_map<ESkill, ClassSkillInfo::Ptr>;
	using SkillsPtr = std::unique_ptr<Skills>;

	SkillsPtr skills;
	int skills_level_decrement{1};

	[[nodiscard]] bool HasSkill(ESkill id) const;
	[[nodiscard]] bool HasntSkill(const ESkill skill_id) const { return !HasSkill(skill_id); };
	[[nodiscard]] int GetMinRemort(ESkill id) const;
	[[nodiscard]] int GetMinLevel(ESkill id) const;
	[[nodiscard]] int GetSkillLevelDecrement() const { return skills_level_decrement; };
	[[nodiscard]] long GetImprove(ESkill id) const;

};

class CharClassInfoBuilder : public info_container::IItemBuilder<CharClassInfo> {
 public:
	ItemOptional Build(parser_wrapper::DataNode &node) final;
 private:
	static parser_wrapper::DataNode SelectDataNode(parser_wrapper::DataNode &node);
	static std::optional<std::string> GetCfgFileName(parser_wrapper::DataNode &node);
	static void ParseClass(ItemOptional &info, parser_wrapper::DataNode &node);
	static void ParseSkills(ItemOptional &info, parser_wrapper::DataNode &node);
	static void ParseSkillsLevelDecrement(ItemOptional &info, parser_wrapper::DataNode &node);
	static void ParseSingleSkill(ItemOptional &info, parser_wrapper::DataNode &node);
	static void ParseSkillVals(ClassSkillInfo::Ptr &info, parser_wrapper::DataNode &node);
};

using ClassesInfo = info_container::InfoContainer<ECharClass, CharClassInfo, CharClassInfoBuilder>;

} // namespace classes

#endif //BYLINS_SRC_CLASSES_CLASSES_INFO_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
