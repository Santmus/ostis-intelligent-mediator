#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_choose_modality (rules/rules_user_adaptation.scs,
// F_rule_adapt):
//   If: rrel_user_situation_context указывает на ограничение визуального
//       восприятия,        Then: concept_modality_voice;
//   Elif: пользователь предпочитает визуальные объяснения
//       (preference_visual), Then: concept_modality_visual;
//   Default: concept_modality_text.
//
// Оба условия ветвления прежде существовали только в прозе правила --
// ни concept_user_situation_context, ни concept_user_preference не
// имели типизированных подклассов, различающих "с ограничением
// восприятия"/"обычный" или "визуальное предпочтение"/прочее.
// Формализованы вместе с этим агентом:
//   - concept_visual_perception_constraint <= nrel_inclusion:
//     concept_user_situation_context (concepts/contexts.scs) -- узел C_u
//     дополнительно классифицируется этим подклассом, когда
//     соответствующая ситуация ограничивает визуальное восприятие;
//   - concept_preference_visual_modality <= nrel_inclusion:
//     concept_user_preference (concepts/users.scs) -- аналогично для
//     предпочтения; существующий сценарный фикстур
//     scenario_intelligent_tutoring.scs (`preference_ivan_visual`)
//     дополнен этой классификацией.
// Оба -- узкие, точечные добавления только для двух веток ЭТОГО
// правила; остальные ситуационные индикаторы, упомянутые в прозе
// соседних правил того же файла (mobile/on-the-go --
// rule_choose_presentation_template; night-shift/low-light --
// rule_context_aware_layout), и остальные виды пользовательских
// предпочтений остаются неформализованными.
//
// Аргументы sc-действия action_choose_modality:
//   1) situation    -- ScAddr узла C_u (concept_user_situation_context),
//                       непосредственно связанного с интеракционным
//                       контекстом через rrel_user_situation_context;
//                       передаётся напрямую вызывающей стороной, а не
//                       выводится обходом от concept_interaction_context
//                       -- тот же приём, что и request у
//                       AgentValidateRightToExplanation
//   2) user_model   -- ScAddr узла concept_user_model, чьи предпочтения
//                       (nrel_has_preference) сканируются на наличие
//                       concept_preference_visual_modality
//   3) result_level -- узел, под которым размещается выбранная
//                       модальность (как дочерний ConstPermPosArc)
//
// Результат -- НЕ числовой индикатор (как у AgentValidate*/
// AgentEvaluateProgressiveDisclosure), а сама выбранная КЛАССОВАЯ
// модальность (concept_modality_voice/_visual/_text, уже существующие
// классы concepts/adaptive_interface.scs). Тот же приём, что у
// AgentBuildExplanation: FormResult возвращает две произвольные
// ScAddr, а не (сущность, числовой value_link).
//
// Не реализовано:
//  - nrel_uses_modality(interface, modality) не пишется -- ни один
//    агент проекта не создаёт экземпляр concept_adaptive_interface (тот
//    же класс ограничения, что у AgentEvaluateProgressiveDisclosure и
//    его nrel_applies_strategy);
//  - "первое найденное" против "любое" предпочтение -- агент
//    останавливается на ПЕРВОМ найденном экземпляре
//    concept_preference_visual_modality среди предпочтений user_model
//    (порядок обхода не гарантирован API sc-machine, но семантически
//    неважен -- достаточно факта существования хотя бы одного);
//  - только IsElement на аргументах, как и у остальных агентов модуля
//    (классы situation/user_model не проверяются, только их вторичная
//    классификация, релевантная для веток правила).
class AgentChooseModality : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
