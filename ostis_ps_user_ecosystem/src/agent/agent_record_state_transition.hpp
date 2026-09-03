#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует action_record_state_transition
// (rules/rules_mediation_orchestration.scs) -- механический учёт перехода
// состояния запроса в конвейере медиации, операционализирующий
// constraint_transition_provenance и constraint_separate_resource_decrease
// (axioms/axioms_constraints.scs) для уже покрытого 26 агентами участка
// конвейера: x_exec/x_aggregate/x_verify и ребро переназначения
// x_exec -> x_assign (AGENTS_AUDIT.md §7.1, план Фазы 1). Агент НЕ решает,
// когда и на какое состояние переходить -- целевой класс передаёт
// вызывающая сторона, тот же принцип, что и у остальных 26 агентов
// модуля (ни один агент проекта не инициирует действие другого агента --
// композиция происходит извне, см. tests/test_integration_request_pipeline.cpp).
// Агент также не перепроверяет содержательное предусловие перехода
// (например, что все подзадачи T(q) реально завершены) -- только граф
// допустимости, ресурсное ограничение и полноту пакета перехода
// (по прецеденту AgentReassignOnResourceExhaustion -- содержательные
// предусловия остаются на стороне вызывающего кода).
//
// Аргументы sc-действия action_record_state_transition:
//   1) request           -- concept_user_request, чей текущий
//                            nrel_has_state читается и переписывается
//   2) to_state_class     -- целевой класс состояния (concept_state_*);
//                            в рамках Фазы 1 распознаются переходы ИЗ
//                            concept_state_execution/_assignment/
//                            _aggregation/_verification (LifecycleKeynodes)
//   3) transition_event   -- уже типизированный узел-событие
//                            (concept_transition_event и подклассы,
//                            например concept_event_reassign) --
//                            вызывающая сторона отвечает за тип, агент
//                            только связывает его с переходом
//   4) plan_version       -- текущий concept_plan_version запроса,
//                            записывается как nrel_uses_plan_version
//                            перехода (constraint_transition_provenance)
//
// Проверки допустимости (в этом порядке, любая непройденная --
// FinishWithError, факты не создаются):
//   a) у request есть текущее состояние (nrel_has_state);
//   b) класс текущего состояния распознан как один из 4 классов Фазы 1;
//   c) ребро (fromClass, toClass) присутствует в nrel_transitions_to
//      (граф допустимости состояний, mediation_lifecycle.scs);
//   d) на текущем состоянии определены оба ресурсных значения
//      (nrel_remaining_reassignment_budget, nrel_remaining_replanning_depth);
//   e) на ребре x_exec -> x_assign: nrel_remaining_reassignment_budget > 0
//      (constraint_separate_resource_decrease).
//
// Результат при успехе: новый экземпляр состояния (классифицирован
// to_state_class, оба ресурса перенесены -- b_rem уменьшается на 1
// ТОЛЬКО на ребре x_exec -> x_assign, d_rem не меняется ни на одном
// ребре Фазы 1); request.nrel_has_state переписан на новый экземпляр
// (SetFunctionalRelationTarget); узел перехода
// (concept_state_transition, rrel_from_state/rrel_to_state/
// rrel_transition_event, nrel_caused_by_event/nrel_uses_plan_version/
// nrel_transition_recorded_in); запись аудита (concept_audit_record +
// concept_audit_record_transition) с nrel_has_provenance (по прецеденту
// AgentRegisterPartialResult и по образцу scenario_lifecycle_recovery.scs).
//
// Не реализовано (сознательно, вне Фазы 1):
//  - rrel_transition_actor/rrel_context на узле перехода -- не входят
//    в минимальный обязательный пакет constraint_transition_provenance;
//    можно добавить как необязательные аргументы позже без изменения
//    уже написанного кода (план Фазы 1);
//  - классы состояний вне 4 перечисленных (x_accepted, x_interpretation,
//    x_planning, x_replanning, x_presentation, x_partial, x_clarification,
//    x_refusal, x_cancelled) и цикл перепланирования d_rem/d_max --
//    Фаза 2 (агенты §7.5, которых сейчас нет);
//  - агент не вызывает и не инициирует другие sc-агенты.
class AgentRecordStateTransition : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
