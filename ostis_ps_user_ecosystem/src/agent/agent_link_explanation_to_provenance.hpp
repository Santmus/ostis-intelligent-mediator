#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_link_explanation_to_provenance (rules/rules_provenance_completion.scs,
// agent_provenance_registrar): для акта построения объяснения
// (concept_explanation_construction_act, порождается AgentBuildExplanation)
// связывает саму сущность объяснения и деятельность её построения с
// использованными результатами и применёнными правилами через уже
// существующие PROV-O отношения (concepts/provenance.scs) -- ровно те,
// которые именует текст правила, без дополнительной типизации целей
// (см. "Не реализовано" ниже).
//
// Аргументы sc-действия action_link_explanation_to_provenance:
//   1) construction_act        -- concept_explanation_construction_act,
//                                  источник ролей rrel_uses_results/
//                                  rrel_applies_rules (обе роли уже
//                                  проставлены AgentBuildExplanation);
//                                  обязателен
//   2) explanation_entity      -- e, цель wasDerivedFrom/wasAttributedTo;
//                                  вызывающая сторона передаёт готовый
//                                  узел (например, explanation, возвращённый
//                                  AgentBuildExplanation, либо отдельный
//                                  concept_prov_entity, зарегистрированный
//                                  через AgentRecordProvEntity) -- этот
//                                  агент его не создаёт и не типизирует;
//                                  обязателен
//   3) explanation_activity    -- activity_of_e, источник PROV:used;
//                                  вызывающая сторона передаёт готовый
//                                  concept_prov_activity (например, через
//                                  AgentRecordProvActivity) -- тот же
//                                  принцип "agent_of()/activity_of() не
//                                  вычисляется агентом", что и в
//                                  AgentRecordProvActivity; обязателен
//   4) builder_prov_agent      -- concept_prov_agent, соответствующий
//                                  agent_explanation_builder; вызывающая
//                                  сторона передаёт готовый узел, та же
//                                  причина, что и (3); обязателен
//
// Результат: для каждой цели r_i роли rrel_uses_results на construction_act
// создаётся wasDerivedFrom(explanation_entity, r_i)
// (nrel_prov_was_derived_from); для каждой цели rule_i роли
// rrel_applies_rules -- PROV:used(explanation_activity, rule_i)
// (nrel_prov_used); один раз -- wasAttributedTo(explanation_entity,
// builder_prov_agent) (nrel_prov_was_attributed_to). Если construction_act
// не имеет целей у одной из ролей (например, для counterfactual, где
// use_results/applies_rules могут быть не заполнены, а based_on --
// заполнен) -- соответствующий цикл просто ничего не пишет, это не
// ошибка (тот же принцип "нет данных = ничего не пишем", что и в
// AgentBuildExplanation).
//
// Не реализовано:
//  - домен nrel_prov_was_derived_from/nrel_prov_used формально требует
//    concept_prov_entity на обеих/второй стороне (concepts/provenance.scs),
//    но r_i (concept_processing_result) и rule_i (concept_sc_rule) этим
//    агентом не типизируются как concept_prov_entity -- ровно то, что
//    AgentBuildExplanation.Limitations назвал "означало бы вводить
//    непроверенную онтологию"; текст самого правила называет целью
//    именно r_i/rule_i напрямую (не какую-то производную PROV-сущность),
//    поэтому агент связывает их как есть, не изобретая дополнительный
//    уровень обёрток;
//  - agent_of(agent_explanation_builder) (отображение sc-агента в
//    concept_prov_agent) не вычисляется -- готовый узел передаётся
//    аргументом (3), тот же приём, что и agent/prov_agent в
//    AgentRecordProvActivity;
//  - только IsElement на всех четырёх аргументах, класс не проверяется --
//    как и у остальных агентов модуля.
class AgentLinkExplanationToProvenance : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
