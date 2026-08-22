#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_register_partial_result
// (ostis_kb_user_ecosystem/rules/rules_cooperation.scs):
//   При получении частичного результата r от системы s по подзадаче σ:
//     создать concept_partial_result(r), produces(s, r),
//     caused_by_subtask(r, σ), generated_in_context(r, C);
//     затем установить subtask_status(σ, completed).
//
// В отличие от AgentDispatchReadySubtask, здесь нет условия готовности
// -- правило безусловно фиксирует факт получения результата, поэтому
// действие не спекулятивно и не пишет числовой индикатор: единственный
// путь к неуспеху -- отсутствующий аргумент (FinishWithError).
//
// Аргументы sc-действия action_register_partial_result:
//   1) system   -- concept_intelligent_system, приславшая результат s
//   2) result   -- сам результат r (существующий узел, ещё не
//                  классифицированный как concept_partial_result --
//                  агент добавляет эту классификацию сам)
//   3) subtask  -- подзадача σ, по которой получен результат
//   4) context  -- контекст взаимодействия C
//
// nrel_produces/nrel_caused_by_subtask/nrel_generated_in_context не
// функциональные отношения (relations/relations_execution.scs,
// relations/relations_context.scs), поэтому создаются безусловно через
// LinkByRelation. nrel_subtask_status -- функциональное отношение (у
// подзадачи не может быть двух статусов одновременно), поэтому перед
// установкой completed прежняя дуга статуса стирается
// (SetFunctionalRelationTarget, common/relation_io.hpp) -- тот же приём,
// что и в AgentDispatchReadySubtask при переходе в in_progress.
//
// wasGeneratedBy/wasAttributedTo (PROV-O часть того же шаблона,
// axioms/axioms_provenance.scs) сюда не входят -- по тексту правила это
// зона ответственности agent_provenance_registrar, а не
// agent_cooperative_executor.
class AgentRegisterPartialResult : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
