#include "agent_update_trust_reliability.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/trust_keynodes.hpp"

using trust_utils::IsUnitInterval;
using trust_utils::ReadFloatLinkContent;
using trust_utils::WriteFloatLinkContent;

float const AgentUpdateTrustReliability::ms_alpha = 0.10f;

ScAddr AgentUpdateTrustReliability::GetActionClass() const
{
  return TrustKeynodes::action_update_trust_reliability;
}

ScResult AgentUpdateTrustReliability::DoProgram(ScAction & action)
{
  auto const & [trustLevelAddr, outcomeLinkAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(trustLevelAddr) || !m_context.IsElement(outcomeLinkAddr))
  {
    m_logger.Error("AgentUpdateTrustReliability: action does not have both arguments.");
    return action.FinishWithError();
  }

  float outcome = 0.0f;
  if (!ReadFloatLinkContent(m_context, outcomeLinkAddr, outcome))
  {
    m_logger.Error("AgentUpdateTrustReliability: outcome argument is not a numeric sc-link.");
    return action.FinishWithError();
  }

  if (!IsUnitInterval(outcome))
  {
    m_logger.Error("AgentUpdateTrustReliability: outcome argument is out of range [0, 1].");
    return action.FinishWithError();
  }

  ScAddr valueLinkAddr;
  ScIterator3Ptr const it3 =
      m_context.CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (it3->Next())
    valueLinkAddr = it3->Get(2);

  float currentTRel = 0.0f;
  if (m_context.IsElement(valueLinkAddr))
  {
    ReadFloatLinkContent(m_context, valueLinkAddr, currentTRel);
  }
  else
  {
    valueLinkAddr = m_context.GenerateLink(ScType::ConstNodeLink);
    m_context.GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, valueLinkAddr);
  }

  float const newTRel = (1.0f - ms_alpha) * currentTRel + ms_alpha * outcome;
  WriteFloatLinkContent(m_context, valueLinkAddr, newTRel);

  action.FormResult(trustLevelAddr, valueLinkAddr);
  m_logger.Debug("AgentUpdateTrustReliability: t_rel updated from ", currentTRel, " to ", newTRel);

  return action.FinishSuccessfully();
}
