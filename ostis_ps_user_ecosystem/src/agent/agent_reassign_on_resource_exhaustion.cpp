#include "agent_reassign_on_resource_exhaustion.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/coalition_keynodes.hpp"

using trust_utils::SetFunctionalRelationTarget;
using trust_utils::WriteFloatLinkContent;

ScAddr AgentReassignOnResourceExhaustion::GetActionClass() const
{
  return CoalitionKeynodes::action_reassign_on_resource_exhaustion;
}

ScResult AgentReassignOnResourceExhaustion::DoProgram(ScAction & action)
{
  auto const & [systemAddr, resultLevelAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(systemAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentReassignOnResourceExhaustion: action does not have both arguments.");
    return action.FinishWithError();
  }

  float reassignedCount = 0.0f;
  ScIterator5Ptr const assignedIt = m_context.CreateIterator5(
      ScType::Unknown,
      ScType::ConstCommonArc,
      systemAddr,
      ScType::ConstPermPosArc,
      CoalitionKeynodes::nrel_assigned_to);
  while (assignedIt->Next())
  {
    ScAddr const subtaskAddr = assignedIt->Get(0);
    SetFunctionalRelationTarget(
        m_context, subtaskAddr, CoalitionKeynodes::nrel_subtask_status,
        CoalitionKeynodes::concept_subtask_status_reassigned);
    reassignedCount += 1.0f;
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
  WriteFloatLinkContent(m_context, valueLinkAddr, reassignedCount);

  action.FormResult(systemAddr, valueLinkAddr);
  m_logger.Debug("AgentReassignOnResourceExhaustion: reassigned = ", reassignedCount);

  return action.FinishSuccessfully();
}
