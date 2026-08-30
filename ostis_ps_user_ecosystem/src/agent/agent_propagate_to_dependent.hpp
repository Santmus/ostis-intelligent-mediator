#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_propagate_to_dependent
// (ostis_kb_user_ecosystem/rules/rules_cooperation.scs), исправленное
// по AGENTS_AUDIT.md п.6.10 (подзадача сама выступала адресатом
// коммуникативного акта, хотя подзадача не является участником
// коммуникации):
//   Для каждой подзадачи σ' такой, что depends_on(σ', σ) и
//   subtask_status(σ, completed), сделать частичный результат r
//   входом σ' через is_input_for(r, σ'); если σ' уже назначена
//   исполнителю s', дополнительно уведомить s' knowledge exchange
//   act'ом с rrel_payload(r).
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
// Для каждой найденной σ' всегда устанавливается is_input_for(r, σ')
// (relations_execution.scs) -- семантическая связь "результат -- вход
// подзадачи", не зависящая от того, назначен ли уже исполнитель.
// Коммуникативный concept_knowledge_exchange_act (rrel_sender=
// agent_cooperative_executor, rrel_act_type=concept_act_inform,
// rrel_payload=result) создаётся дополнительно и только когда у σ' уже
// есть nrel_assigned_to -- получателем становится сама система-
// исполнитель, а не подзадача: до назначения (task_allocator ещё не
// отработал) участника коммуникации попросту нет, и связи is_input_for
// достаточно, чтобы результат считался пропагированным.
class AgentPropagateToDependent : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
