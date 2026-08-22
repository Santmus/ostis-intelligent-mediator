#include "agent_propagate_to_dependent.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/task_keynodes.hpp"

using trust_utils::LinkByRole;
using trust_utils::TargetOf;
using trust_utils::WriteFloatLinkContent;

namespace
{
void NotifyDependent(ScAgentContext & context, ScAddr const & dependentAddr, ScAddr const & resultAddr)
{
  ScAddr const actAddr = context.GenerateNode(ScType::ConstNode);
  context.GenerateConnector(ScType::ConstPermPosArc, TaskKeynodes::concept_knowledge_exchange_act, actAddr);
  LinkByRole(context, actAddr, TaskKeynodes::agent_cooperative_executor, TaskKeynodes::rrel_sender);
  LinkByRole(context, actAddr, dependentAddr, TaskKeynodes::rrel_receiver);
  LinkByRole(context, actAddr, TaskKeynodes::concept_act_inform, TaskKeynodes::rrel_act_type);
  LinkByRole(context, actAddr, resultAddr, TaskKeynodes::rrel_payload);
}
}  // namespace

ScAddr AgentPropagateToDependent::GetActionClass() const
{
  return TaskKeynodes::action_propagate_to_dependent;
}

ScResult AgentPropagateToDependent::DoProgram(ScAction & action)
{
  auto const & [resultAddr, subtaskAddr, resultLevelAddr] = action.GetArguments<3>();
  if (!m_context.IsElement(resultAddr) || !m_context.IsElement(subtaskAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentPropagateToDependent: action does not have all three arguments.");
    return action.FinishWithError();
  }

  ScAddr const statusAddr = TargetOf(m_context, subtaskAddr, TaskKeynodes::nrel_subtask_status);
  bool const completed = statusAddr == TaskKeynodes::concept_subtask_status_completed;

  float propagatedCount = 0.0f;
  if (completed)
  {
    ScIterator5Ptr const dependentsIt = m_context.CreateIterator5(
        ScType::Unknown,
        ScType::ConstCommonArc,
        subtaskAddr,
        ScType::ConstPermPosArc,
        TaskKeynodes::nrel_subtask_depends_on);
    while (dependentsIt->Next())
    {
      NotifyDependent(m_context, dependentsIt->Get(0), resultAddr);
      propagatedCount += 1.0f;
    }
  }

  ScAddr valueLinkAddr;
  ScIterator3Ptr const it3 =
      m_context.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (it3->Next())
    valueLinkAddr = it3->Get(2);
  else
  {
    valueLinkAddr = m_context.GenerateLink(ScType::ConstNodeLink);
    m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, valueLinkAddr);
  }
  WriteFloatLinkContent(m_context, valueLinkAddr, propagatedCount);

  action.FormResult(subtaskAddr, valueLinkAddr);
  m_logger.Debug("AgentPropagateToDependent: propagated to ", propagatedCount, " dependent subtask(s).");

  return action.FinishSuccessfully();
}
