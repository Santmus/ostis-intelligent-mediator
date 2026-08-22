#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_propagate_to_dependent
// (ostis_kb_user_ecosystem/rules/rules_cooperation.scs):
//   Для каждой подзадачи σ' такой, что depends_on(σ', σ) и
//   subtask_status(σ, completed), сделать частичный результат r
//   доступным как вход σ' через создание knowledge exchange act с
//   rrel_payload(r).
//
// nrel_subtask_depends_on хранится как σ -[depends_on]-> σ'
// (см. AllDependenciesCompleted в agent_dispatch_ready_subtask.cpp),
// т.е. σ зависит от σ'. Правило же требует depends_on(σ', σ) -- σ'
// зависит от только что завершённой σ -- поэтому здесь нужен обратный
// обход: ищутся все σ' с исходящей дугой depends_on именно на σ
// (переданный аргумент subtask), а не наоборот.
//
// Как и AgentDispatchReadySubtask, агент безопасен для спекулятивного
// вызова: если σ ещё не completed, действие ничего не пропагирует и
// пишет индикатор 0 -- это не ошибка, а обычный результат вызова "рано".
//
// Аргументы sc-действия action_propagate_to_dependent:
//   1) result       -- частичный результат r, полученный по σ
//   2) subtask      -- только что завершённая подзадача σ
//   3) result_level -- узел-приёмник индикатора: число подзадач σ',
//                      которым был пропагирован результат (0, если σ
//                      ещё не completed или зависимых σ' нет); если у
//                      него ещё нет числовой sc-ссылки, она создаётся
//
// Для каждой найденной σ' создаётся concept_knowledge_exchange_act с
// rrel_sender=agent_cooperative_executor, rrel_receiver=σ',
// rrel_act_type=concept_act_inform, rrel_payload=result -- получателем
// взята сама подзадача σ', а не назначенная ей система: на момент
// пропагации σ' может быть ещё не назначена (nrel_assigned_to
// отсутствует, task_allocator ещё не отработал), а правило требует
// именно "сделать результат входом σ'", то есть адресат по смыслу --
// сама подзадача.
class AgentPropagateToDependent : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
