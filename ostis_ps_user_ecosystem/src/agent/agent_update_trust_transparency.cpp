#include "agent_update_trust_transparency.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/trust_keynodes.hpp"

using trust_utils::IsUnitInterval;
using trust_utils::ReadFloatLinkContent;
using trust_utils::WriteFloatLinkContent;

// Коэффициент сглаживания eta (единый для всех четырёх EMA-агентов по
// пересмотренной главе 2, part2.tex): значение по умолчанию 0.10;
// передаётся необязательным 3-м аргументом действия (как k_sigma в
// AgentDetectAnomalousRequestRate), т.к. eta -- калибруемый параметр
// модели, замораживаемый перед оценочным прогоном.
float const AgentUpdateTrustTransparency::ms_eta = 0.10f;

ScAddr AgentUpdateTrustTransparency::GetActionClass() const
{
  return TrustKeynodes::action_update_trust_transparency;
}

ScResult AgentUpdateTrustTransparency::DoProgram(ScAction & action)
{
  auto const & [trustLevelAddr, justificationLinkAddr, etaLinkAddr] = action.GetArguments<3>();
  if (!m_context.IsElement(trustLevelAddr) || !m_context.IsElement(justificationLinkAddr))
  {
    m_logger.Error("AgentUpdateTrustTransparency: action does not have both required arguments.");
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

  float eta = ms_eta;
  if (m_context.IsElement(etaLinkAddr))
  {
    if (!ReadFloatLinkContent(m_context, etaLinkAddr, eta) || eta <= 0.0f || eta >= 1.0f)
    {
      m_logger.Error("AgentUpdateTrustTransparency: eta argument is not a numeric sc-link in (0, 1).");
      return action.FinishWithError();
    }
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

  float const newTTr = (1.0f - eta) * currentTTr + eta * justification;
  WriteFloatLinkContent(m_context, valueLinkAddr, newTTr);

  action.FormResult(trustLevelAddr, valueLinkAddr);
  m_logger.Debug("AgentUpdateTrustTransparency: t_tr updated from ", currentTTr, " to ", newTTr);

  return action.FinishSuccessfully();
}
