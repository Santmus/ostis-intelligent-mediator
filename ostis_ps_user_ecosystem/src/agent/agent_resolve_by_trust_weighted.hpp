#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_resolve_by_trust_weighted
// (ostis_kb_user_ecosystem/rules/rules_integration.scs):
//   w_i = trust(s_i, tau, h) для каждого кандидата r_i;
//   r* = arg max_i w_i;
//   зафиксировать resolves_conflict(ir, c).
//
// Единственный из реализованных агентов, напрямую переиспользующий
// готовый trust(s,tau,h) -- вес каждого кандидата ищется по цепочке
// уже существующих отношений онтологии: r_i -[nrel_produced_by]-> s_i
// -[nrel_has_trust_level]-> trust_level -> value_link (тот же узел,
// который пишет AgentAggregateTrust).
//
// Аргументы sc-действия action_resolve_by_trust_weighted:
//   1) candidates_group -- узел с исходящими положительными постоянными
//                          дугами (ConstPermPosArc) к конфликтующим
//                          экземплярам concept_processing_result
//   2) conflict          -- существующий экземпляр concept_result_conflict,
//                          который разрешается этим выбором
//
// Результат: создаётся новый экземпляр concept_integrated_result (ir),
// связанный с победившим кандидатом r* через nrel_integrated_in
// (r* -> ir) и с переданным conflict через nrel_resolves_conflict
// (ir -> conflict). В результат действия включаются ir и r*.
//
// Пустая candidates_group -- ошибка (FinishWithError), а не нейтральный
// результат: в отличие от числовых метрик (AgentComputePiMetric,
// AgentAggregateTrust), здесь нет осмысленного "нулевого" победителя --
// выбирать не из чего.
//
// Кандидат без доверительной цепочки (нет nrel_produced_by, либо у
// системы нет nrel_has_trust_level, либо у trust_level нет числовой
// sc-ссылки) получает вес 0 -- тот же принцип "отсутствие данных = 0",
// что и в AgentAggregateTrust.
//
// Не реализовано: формирование текстового обоснования выбора
// ("с обоснованием" в тексте правила) -- это по сути задача
// agent_explanation_builder, не реализованного в проекте.
class AgentResolveByTrustWeighted : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
