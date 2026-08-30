#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_resolve_by_trust_weighted
// (ostis_kb_user_ecosystem/rules/rules_integration.scs):
//   w_i = trust(s_i, tau, h) для каждого кандидата r_i;
//   r* = arg max_i w_i;
//   зафиксировать разрешение конфликта актом, а не напрямую результатом
//   (см. relations/relations_trust.scs, "РАСХОЖДЕНИЕ С РЕАЛИЗАЦИЕЙ" --
//   этой правкой снимается).
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
// (r* -> ir). Конфликт разрешается овеществлённым актом (act) --
// concept_conflict_resolution_act, специализированным как
// concept_resolution_by_trust_weighting -- через conflict
// -[nrel_resolved_by]-> act, а его статус переводится в
// concept_conflict_status_resolved (nrel_conflict_status, функциональное
// отношение). resolves_conflict относится не к ir, а к порождённому
// объяснению (explanation -[nrel_resolves_conflict]-> conflict), которое
// также связывается с ir через nrel_explained_by -- в соответствии
// с итоговой формализацией домена этого отношения. В результат
// действия включаются ir, r*, act и explanation.
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
// Не реализовано: содержательное текстовое обоснование выбора внутри
// explanation -- порождённый узел объяснения структурно корректен
// (типизирован, связан с ir и conflict), но без текста; наполнение --
// задача agent_explanation_builder, не реализованного в проекте.
class AgentResolveByTrustWeighted : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
