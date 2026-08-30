#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_register_partial_result
// (ostis_kb_user_ecosystem/rules/rules_cooperation.scs), дополненное по
// AGENTS_AUDIT.md п.6.2 (агент безусловно завершал подзадачу без
// проверки допуска и без акта исполнения/аудита/провенанса):
//   При получении частичного результата r от системы s по подзадаче σ,
//   если s -- назначенный исполнитель σ и σ уже in_progress:
//     создать concept_partial_result(r), concept_processing_result(r),
//     produces(s, r), caused_by_subtask(r, σ), generated_in_context(r, C);
//     создать акт исполнения a (concept_execution_act, успешный),
//     executes(a, σ); создать запись аудита υ (concept_audit_record,
//     подтип _execution), recorded_in(a, υ); generated_by(r, a);
//     снабдить провенансной записью и r, и υ (has_provenance);
//     затем установить subtask_status(σ, completed).
//   Иначе -- FinishWithError: подзадача остаётся в прежнем статусе.
//
// В отличие от AgentDispatchReadySubtask, здесь нет условия готовности
// в смысле числового индикатора -- допуск проверяется два раза
// (исполнитель, статус) и при провале действие завершается ошибкой, не
// пишет частичный факт.
//
// Аргументы sc-действия action_register_partial_result:
//   1) system   -- concept_intelligent_system, приславшая результат s;
//                  должна совпадать с nrel_assigned_to подзадачи
//   2) result   -- сам результат r (существующий узел, ещё не
//                  классифицированный как concept_partial_result --
//                  агент добавляет эту классификацию сам)
//   3) subtask  -- подзадача σ, по которой получен результат; должна
//                  быть в статусе concept_subtask_status_in_progress
//   4) context  -- контекст взаимодействия C
//
// nrel_produces/nrel_caused_by_subtask/nrel_generated_in_context/
// nrel_executes/nrel_generated_by не функциональны или всюду
// определены на новых узлах (создаются безусловно через LinkByRelation).
// nrel_recorded_in и nrel_has_provenance функциональны
// (concepts/execution_acts.scs, relations/relations_provenance.scs), но
// пишутся на только что созданные a/υ/r -- прежней дуги там нет, поэтому
// LinkByRelation достаточно. nrel_subtask_status -- функциональное
// отношение, поэтому прежняя дуга статуса стирается
// (SetFunctionalRelationTarget, common/relation_io.hpp).
//
// Провенансная запись минимальна: типизирована и связана
// (nrel_recorded_by_sc_agent -> agent_cooperative_executor), без полного
// PROV-O триплета Entity/Activity/Agent -- это по-прежнему зона
// ответственности agent_provenance_registrar, не реализованного
// в проекте. Версия плана, реальное время исполнения и проверка
// целостности передачи результата (остальные пункты AGENTS_AUDIT.md
// п.6.2) также не входят в минимальный набор, который сам аудит
// определяет как обязательный ("как минимум").
class AgentRegisterPartialResult : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
