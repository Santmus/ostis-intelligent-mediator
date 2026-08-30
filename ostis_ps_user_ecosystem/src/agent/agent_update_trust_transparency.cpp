#include "agent_update_trust_transparency.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/trust_keynodes.hpp"

using trust_utils::IsUnitInterval;
using trust_utils::ReadFloatLinkContent;
using trust_utils::WriteFloatLinkContent;

float const AgentUpdateTrustTransparency::ms_delta = 0.10f;

ScAddr AgentUpdateTrustTransparency::GetActionClass() const
{
  return TrustKeynodes::action_update_trust_transparency;
}

ScResult AgentUpdateTrustTransparency::DoProgram(ScAction & action)
{
  auto const & [trustLevelAddr, justificationLinkAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(trustLevelAddr) || !m_context.IsElement(justificationLinkAddr))
  {
    m_logger.Error("AgentUpdateTrustTransparency: action does not have both arguments.");
    return action.FinishWithError();
  }

  float justification = 0.0f;
  if (!ReadFloatLinkContent(m_context, justificationLinkAddr, justification))
  {
    m_logger.Error("AgentUpdateTrustTransparency: justification argument is not a numeric sc-link.");
    return action.FinishWithError();
  }

  if (!IsUnitInterval(justification))
  {
    m_logger.Error("AgentUpdateTrustTransparency: justification argument is out of range [0, 1].");
    return action.FinishWithError();
  }

  ScAddr valueLinkAddr;
  ScIterator3Ptr const it3 =
      m_context.CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (it3->Next())
    valueLinkAddr = it3->Get(2);

  float currentTTr = 0.0f;
  if (m_context.IsElement(valueLinkAddr))
  {
    ReadFloatLinkContent(m_context, valueLinkAddr, currentTTr);
  }
  else
  {
    valueLinkAddr = m_context.GenerateLink(ScType::ConstNodeLink);
    m_context.GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, valueLinkAddr);
  }

  float const newTTr = (1.0f - ms_delta) * currentTTr + ms_delta * justification;
  WriteFloatLinkContent(m_context, valueLinkAddr, newTTr);

  action.FormResult(trustLevelAddr, valueLinkAddr);
  m_logger.Debug("AgentUpdateTrustTransparency: t_tr updated from ", currentTTr, " to ", newTTr);

  return action.FinishSuccessfully();
}
