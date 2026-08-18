#include "agent_update_trust_accuracy.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/trust_keynodes.hpp"

using trust_utils::ReadFloatLinkContent;
using trust_utils::WriteFloatLinkContent;

float const AgentUpdateTrustAccuracy::ms_beta = 0.10f;

ScAddr AgentUpdateTrustAccuracy::GetActionClass() const
{
  return TrustKeynodes::action_update_trust_accuracy;
}

ScResult AgentUpdateTrustAccuracy::DoProgram(ScAction & action)
{
  auto const & [trustLevelAddr, qualityLinkAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(trustLevelAddr) || !m_context.IsElement(qualityLinkAddr))
  {
    m_logger.Error("AgentUpdateTrustAccuracy: action does not have both arguments.");
    return action.FinishWithError();
  }

  float quality = 0.0f;
  if (!ReadFloatLinkContent(m_context, qualityLinkAddr, quality))
  {
    m_logger.Error("AgentUpdateTrustAccuracy: quality argument is not a numeric sc-link.");
    return action.FinishWithError();
  }

  ScAddr valueLinkAddr;
  ScIterator3Ptr const it3 =
      m_context.CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (it3->Next())
    valueLinkAddr = it3->Get(2);

  float currentTAcc = 0.0f;
  if (m_context.IsElement(valueLinkAddr))
  {
    ReadFloatLinkContent(m_context, valueLinkAddr, currentTAcc);
  }
  else
  {
    valueLinkAddr = m_context.GenerateLink(ScType::ConstNodeLink);
    m_context.GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, valueLinkAddr);
  }

  float const newTAcc = (1.0f - ms_beta) * currentTAcc + ms_beta * quality;
  WriteFloatLinkContent(m_context, valueLinkAddr, newTAcc);

  action.FormResult(trustLevelAddr, valueLinkAddr);
  m_logger.Debug("AgentUpdateTrustAccuracy: t_acc updated from ", currentTAcc, " to ", newTAcc);

  return action.FinishSuccessfully();
}
