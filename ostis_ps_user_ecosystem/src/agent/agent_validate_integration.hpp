#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_validate_integration (rules/rules_integration.scs):
//   Перед фиксацией интегрированного результата ir убедиться, что:
//   (a) ∃e. explained_by(ir, e);  (b) ¬∃c. conflicts_with(ir, c).
// Обе связи уже существуют в онтологии: nrel_explained_by (ориентированное,
// домен concept_result -> concept_explanation,
// relations/relations_provenance.scs) и nrel_conflicts_with (симметричное,
// антирефлексивное, relations/relations_trust.scs). Первое условие теперь
// содержательно, а не тавтологически ложно: [[AgentBuildExplanation]]
// реально устанавливает nrel_explained_by для explanandum, классифицированного
// concept_integrated_result.
//
// Аргументы sc-действия action_validate_integration:
//   1) integrated_result -- проверяемый экземпляр concept_integrated_result
//                            (не типизация не проверяется -- как и везде
//                            в модуле, агент доверяет вызывающей стороне)
//   2) result_level       -- узел, под которым размещается индикатор
//
// nrel_conflicts_with объявлено симметричным, но в сценариях KB
// материализовано непоследовательно: scenario_collective_decision.scs
// создаёт обе дуги (r1->r2 и r2->r1), а scenario_smart_city_traffic.scs --
// только одну. Поэтому агент проверяет наличие связи в ОБОИХ направлениях
// (исходящую и входящую), а не полагается на то, что симметрия всегда
// физически продублирована.
//
// Результат: valid = has_explanation && !has_conflict записывается как
// 1.0/0.0 в value_link узла result_level (тот же приём, что у
// AgentDispatchReadySubtask/AgentDetectAnomalousRequestRate).
//
// Не реализовано:
//  - "иначе вернуть управление rule_resolve_by_*" -- агент только
//    вычисляет индикатор, не инициирует никакой другой rule/агент при
//    невалидности; передача управления -- ответственность внешнего
//    оркестратора, которого в проекте нет (тот же принцип, что и
//    активация agent_coalition_reconfigurator у
//    AgentDetectAnomalousRequestRate);
//  - остальная область ответственности из более широкого §7.5-агента
//    AgentVerifyResult ("проверяет ограничения, объяснение, провенанс и
//    конфликты") -- этот агент закрывает только explanation+conflicts
//    (rule_validate_integration); ограничения покрывает
//    AgentValidateCardinalityConstraint, провенанс -- AgentComputePiMetric,
//    оба независимо и раньше;
//  - только IsElement на аргументах, как и у остальных агентов модуля.
class AgentValidateIntegration : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
