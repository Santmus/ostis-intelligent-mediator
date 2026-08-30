#include "agent_update_trust_security.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/trust_keynodes.hpp"

using trust_utils::IsUnitInterval;
using trust_utils::ReadFloatLinkContent;
using trust_utils::WriteFloatLinkContent;

float const AgentUpdateTrustSecurity::ms_gamma = 0.10f;

ScAddr AgentUpdateTrustSecurity::GetActionClass() const
{
  return TrustKeynodes::action_update_trust_security;
}

ScResult AgentUpdateTrustSecurity::DoProgram(ScAction & action)
{
  auto const & [trustLevelAddr, incidentLinkAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(trustLevelAddr) || !m_context.IsElement(incidentLinkAddr))
  {
    m_logger.Error("AgentUpdateTrustSecurity: action does not have both arguments.");
    return action.FinishWithError();
  }

  float incidentAbsence = 0.0f;
  if (!ReadFloatLinkContent(m_context, incidentLinkAddr, incidentAbsence))
  {
    m_logger.Error("AgentUpdateTrustSecurity: incident indicator argument is not a numeric sc-link.");
    return action.FinishWithError();
  }

  if (!IsUnitInterval(incidentAbsence))
  {
    m_logger.Error("AgentUpdateTrustSecurity: incident indicator argument is out of range [0, 1].");
    return action.FinishWithError();
  }

  ScAddr valueLinkAddr;
  ScIterator3Ptr const it3 =
      m_context.CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (it3->Next())
    valueLinkAddr = it3->Get(2);

  float currentTSec = 0.0f;
  if (m_context.IsElement(valueLinkAddr))
  {
    ReadFloatLinkContent(m_context, valueLinkAddr, currentTSec);
  }
  else
  {
    valueLinkAddr = m_context.GenerateLink(ScType::ConstNodeLink);
    m_context.GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, valueLinkAddr);
  }

  float const newTSec = (1.0f - ms_gamma) * currentTSec + ms_gamma * incidentAbsence;
  WriteFloatLinkContent(m_context, valueLinkAddr, newTSec);

  action.FormResult(trustLevelAddr, valueLinkAddr);
  m_logger.Debug("AgentUpdateTrustSecurity: t_sec updated from ", currentTSec, " to ", newTSec);

  return action.FinishSuccessfully();
}
