#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует механическую часть rule_context_aware_layout
// (rules/rules_user_adaptation.scs, F_rule_adapt):
//   If: rrel_user_situation_context включает индикатор «night-shift»
//       ИЛИ low-light,
//   Then: применить тёмную тему; снизить количество одновременно
//         показываемых элементов; увеличить размер шрифта на 10%.
//
// Оба условия ветвления прежде существовали только в прозе правила --
// concept_user_situation_context не имел подклассов, различающих
// "ночная смена"/"низкая освещённость". Формализованы вместе с этим
// агентом, тем же приёмом, что и concept_visual_perception_constraint у
// AgentChooseModality:
//   - concept_night_shift_context <= nrel_inclusion:
//     concept_user_situation_context (concepts/contexts.scs);
//   - concept_low_light_context <= nrel_inclusion:
//     concept_user_situation_context (concepts/contexts.scs).
// Два отдельных подкласса, а не один общий "низкая видимость" -- правило
// называет их как два самостоятельных условия ("night-shift" ИЛИ
// "low-light"), не эквивалентных по смыслу.
//
// Аргументы sc-действия action_evaluate_context_aware_layout:
//   1) situation    -- ScAddr узла C_u (concept_user_situation_context),
//                       непосредственно связанного с интеракционным
//                       контекстом через rrel_user_situation_context;
//                       передаётся напрямую, тот же приём, что и у
//                       AgentChooseModality
//   2) result_level -- узел, под которым размещаются индикатор и, при
//                       срабатывании, маркер стратегии
//
// Результат -- числовой индикатор (1.0 = условие сработало, 0.0 -- нет),
// тот же приём decision-индикатора, что у AgentEvaluateProgressiveDisclosure
// (НЕ "валидно/невалидно"). При срабатывании под result_level
// присоединяется уже существующий concept_strategy_context_aware_layout
// (concepts/adaptive_interface.scs) -- единый маркер для всего пакета
// последствий (тёмная тема + сокращение элементов + шрифт), а не
// отдельные факты для каждого: у "тёмной темы" и "сокращения элементов"
// нет собственного типизированного представления в KB, а для
// "увеличения шрифта на 10%" нет представления "текущий размер шрифта",
// которое можно было бы умножить. concept_strategy_context_aware_layout
// уже существовало как класс до этого агента (без собственного
// определения, раскрывающего его состав) -- именно этот агент
// операционализирует его впервые.
//
// Не реализовано:
//  - "снизить количество одновременно показываемых элементов" -- цель
//    нигде не квантифицирована в тексте (agents_vault/Implementation
//    Backlog.md), тот же класс блокировки, что у cognitive_load(u)
//    в rule_choose_presentation_template;
//  - "увеличить размер шрифта на 10%" как отдельное численное действие
//    -- нет представления базового размера шрифта для умножения; сама
//    формула (×1.10) не содержит неопределённости, но применять её не
//    к чему без UI-состояния;
//  - nrel_applies_strategy(interface, ...) не пишется -- ни один агент
//    проекта не создаёт экземпляр concept_adaptive_interface, тот же
//    класс ограничения, что у AgentEvaluateProgressiveDisclosure/
//    AgentChooseModality;
//  - только IsElement на аргументах, как и у остальных агентов модуля.
class AgentEvaluateContextAwareLayout : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
