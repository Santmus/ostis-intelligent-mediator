#include "agent_register_partial_result.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/relation_io.hpp"
#include "keynodes/task_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::SetFunctionalRelationTarget;
using trust_utils::TargetOf;

ScAddr AgentRegisterPartialResult::GetActionClass() const
{
  return TaskKeynodes::action_register_partial_result;
}

ScResult AgentRegisterPartialResult::DoProgram(ScAction & action)
{
  auto const & [systemAddr, resultAddr, subtaskAddr, contextAddr] = action.GetArguments<4>();
  if (!m_context.IsElement(systemAddr) || !m_context.IsElement(resultAddr) || !m_context.IsElement(subtaskAddr)
      || !m_context.IsElement(contextAddr))
  {
    m_logger.Error("AgentRegisterPartialResult: action does not have all four arguments.");
    return action.FinishWithError();
  }

  ScAddr const assigneeAddr = TargetOf(m_context, subtaskAddr, TaskKeynodes::nrel_assigned_to);
  if (assigneeAddr != systemAddr)
  {
    m_logger.Error("AgentRegisterPartialResult: system is not the assigned executor of the subtask.");
    return action.FinishWithError();
  }

  ScAddr const statusAddr = TargetOf(m_context, subtaskAddr, TaskKeynodes::nrel_subtask_status);
  if (statusAddr != TaskKeynodes::concept_subtask_status_in_progress)
  {
    m_logger.Error("AgentRegisterPartialResult: subtask is not in_progress.");
    return action.FinishWithError();
  }

  m_context.GenerateConnector(ScType::ConstPermPosArc, TaskKeynodes::concept_partial_result, resultAddr);
  m_context.GenerateConnector(ScType::ConstPermPosArc, TaskKeynodes::concept_processing_result, resultAddr);

  LinkByRelation(m_context, systemAddr, resultAddr, TaskKeynodes::nrel_produces);
  LinkByRelation(m_context, resultAddr, subtaskAddr, TaskKeynodes::nrel_caused_by_subtask);
  LinkByRelation(m_context, resultAddr, contextAddr, TaskKeynodes::nrel_generated_in_context);

  // Акт исполнения и запись аудита (concepts/execution_acts.scs):
  // результат не считается зарегистрированным без обоих --
  // generated_by/recorded_in/has_provenance функциональны и всюду
  // определены (affirmation_execution_act_obligations).
  ScAddr const executionActAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, TaskKeynodes::concept_execution_act, executionActAddr);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, TaskKeynodes::concept_execution_act_successful, executionActAddr);
  LinkByRelation(m_context, executionActAddr, subtaskAddr, TaskKeynodes::nrel_executes);

  ScAddr const auditRecordAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, TaskKeynodes::concept_audit_record, auditRecordAddr);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, TaskKeynodes::concept_audit_record_execution, auditRecordAddr);
  LinkByRelation(m_context, executionActAddr, auditRecordAddr, TaskKeynodes::nrel_recorded_in);

  LinkByRelation(m_context, resultAddr, executionActAddr, TaskKeynodes::nrel_generated_by);

  ScAddr const resultProvenanceAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, TaskKeynodes::concept_provenance_record, resultProvenanceAddr);
  LinkByRelation(m_context, resultAddr, resultProvenanceAddr, TaskKeynodes::nrel_has_provenance);
  LinkByRelation(
      m_context, resultProvenanceAddr, TaskKeynodes::agent_cooperative_executor,
      TaskKeynodes::nrel_recorded_by_sc_agent);

  ScAddr const auditProvenanceAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, TaskKeynodes::concept_provenance_record, auditProvenanceAddr);
  LinkByRelation(m_context, auditRecordAddr, auditProvenanceAddr, TaskKeynodes::nrel_has_provenance);
  LinkByRelation(
      m_context, auditProvenanceAddr, TaskKeynodes::agent_cooperative_executor,
      TaskKeynodes::nrel_recorded_by_sc_agent);

  SetFunctionalRelationTarget(
      m_context, subtaskAddr, TaskKeynodes::nrel_subtask_status, TaskKeynodes::concept_subtask_status_completed);

  action.FormResult(resultAddr, subtaskAddr, executionActAddr, auditRecordAddr);
  m_logger.Debug("AgentRegisterPartialResult: registered partial result and completed subtask.");

  return action.FinishSuccessfully();
}
