#include "agent_exclude_from_coalition.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/coalition_keynodes.hpp"

using trust_utils::TargetOf;
using trust_utils::WriteFloatLinkContent;

ScAddr AgentExcludeFromCoalition::GetActionClass() const
{
  return CoalitionKeynodes::action_exclude_from_coalition;
}

ScResult AgentExcludeFromCoalition::DoProgram(ScAction & action)
{
  auto const & [subtaskAddr, communityAddr, resultLevelAddr] = action.GetArguments<3>();
  if (!m_context.IsElement(subtaskAddr) || !m_context.IsElement(communityAddr)
      || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentExcludeFromCoalition: action does not have all three arguments.");
    return action.FinishWithError();
  }

  ScAddr const systemAddr = TargetOf(m_context, subtaskAddr, CoalitionKeynodes::nrel_assigned_to);
  ScAddr const statusAddr = TargetOf(m_context, subtaskAddr, CoalitionKeynodes::nrel_subtask_status);

  bool excluded = false;
  if (m_context.IsElement(systemAddr) && statusAddr == CoalitionKeynodes::concept_subtask_status_failed)
  {
    ScIterator5Ptr const membershipIt = m_context.CreateIterator5(
        systemAddr,
        ScType::ConstCommonArc,
        communityAddr,
        ScType::ConstPermPosArc,
        CoalitionKeynodes::nrel_member_of);
    if (membershipIt->Next())
    {
      m_context.EraseElement(membershipIt->Get(1));
      excluded = true;
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
  WriteFloatLinkContent(m_context, valueLinkAddr, excluded ? 1.0f : 0.0f);

  action.FormResult(subtaskAddr, valueLinkAddr);
  m_logger.Debug("AgentExcludeFromCoalition: excluded = ", excluded);

  return action.FinishSuccessfully();
}
