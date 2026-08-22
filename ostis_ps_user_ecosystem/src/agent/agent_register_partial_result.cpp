#include "agent_register_partial_result.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/relation_io.hpp"
#include "keynodes/task_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::SetFunctionalRelationTarget;

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

  m_context.GenerateConnector(ScType::ConstPermPosArc, TaskKeynodes::concept_partial_result, resultAddr);

  LinkByRelation(m_context, systemAddr, resultAddr, TaskKeynodes::nrel_produces);
  LinkByRelation(m_context, resultAddr, subtaskAddr, TaskKeynodes::nrel_caused_by_subtask);
  LinkByRelation(m_context, resultAddr, contextAddr, TaskKeynodes::nrel_generated_in_context);

  SetFunctionalRelationTarget(
      m_context, subtaskAddr, TaskKeynodes::nrel_subtask_status, TaskKeynodes::concept_subtask_status_completed);

  action.FormResult(resultAddr, subtaskAddr);
  m_logger.Debug("AgentRegisterPartialResult: registered partial result and completed subtask.");

  return action.FinishSuccessfully();
}
