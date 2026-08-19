#include "agent_dispatch_ready_subtask.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/task_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::TargetOf;
using trust_utils::WriteFloatLinkContent;

namespace
{
void SetFunctionalStatus(ScAgentContext & context, ScAddr const & subtaskAddr, ScAddr const & newStatusAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      subtaskAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      TaskKeynodes::nrel_subtask_status);
  while (it5->Next())
    context.EraseElement(it5->Get(1));

  LinkByRelation(context, subtaskAddr, newStatusAddr, TaskKeynodes::nrel_subtask_status);
}

void LinkByRole(ScAgentContext & context, ScAddr const & actAddr, ScAddr const & targetAddr, ScAddr const & roleAddr)
{
  ScAddr const arcAddr = context.GenerateConnector(ScType::ConstPermPosArc, actAddr, targetAddr);
  context.GenerateConnector(ScType::ConstPermPosArc, roleAddr, arcAddr);
}

bool AllDependenciesCompleted(ScAgentContext & context, ScAddr const & subtaskAddr)
{
  ScIterator5Ptr const depsIt = context.CreateIterator5(
      subtaskAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      TaskKeynodes::nrel_subtask_depends_on);
  while (depsIt->Next())
  {
    ScAddr const dependencyAddr = depsIt->Get(2);
    ScAddr const statusAddr = TargetOf(context, dependencyAddr, TaskKeynodes::nrel_subtask_status);
    if (statusAddr != TaskKeynodes::concept_subtask_status_completed)
      return false;
  }
  return true;
}
}  // namespace

ScAddr AgentDispatchReadySubtask::GetActionClass() const
{
  return TaskKeynodes::action_dispatch_ready_subtask;
}

ScResult AgentDispatchReadySubtask::DoProgram(ScAction & action)
{
  auto const & [subtaskAddr, resultLevelAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(subtaskAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentDispatchReadySubtask: action does not have both arguments.");
    return action.FinishWithError();
  }

  ScAddr const assigneeAddr = TargetOf(m_context, subtaskAddr, TaskKeynodes::nrel_assigned_to);
  ScAddr const statusAddr = TargetOf(m_context, subtaskAddr, TaskKeynodes::nrel_subtask_status);

  bool const ready = m_context.IsElement(assigneeAddr)
                      && statusAddr == TaskKeynodes::concept_subtask_status_proposed
                      && AllDependenciesCompleted(m_context, subtaskAddr);

  if (ready)
  {
    SetFunctionalStatus(m_context, subtaskAddr, TaskKeynodes::concept_subtask_status_in_progress);

    ScAddr const actAddr = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(ScType::ConstPermPosArc, TaskKeynodes::concept_knowledge_exchange_act, actAddr);
    LinkByRole(m_context, actAddr, TaskKeynodes::agent_cooperative_executor, TaskKeynodes::rrel_sender);
    LinkByRole(m_context, actAddr, assigneeAddr, TaskKeynodes::rrel_receiver);
    LinkByRole(m_context, actAddr, TaskKeynodes::concept_act_request, TaskKeynodes::rrel_act_type);
    LinkByRole(m_context, actAddr, subtaskAddr, TaskKeynodes::rrel_payload);
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
  WriteFloatLinkContent(m_context, valueLinkAddr, ready ? 1.0f : 0.0f);

  action.FormResult(subtaskAddr, valueLinkAddr);
  m_logger.Debug("AgentDispatchReadySubtask: subtask dispatched = ", ready);

  return action.FinishSuccessfully();
}
