#include "agent_detect_anomalous_request_rate.hpp"

#include <limits>

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/security_keynodes.hpp"

using trust_utils::ReadFloatLinkContent;
using trust_utils::WriteFloatLinkContent;

ScAddr AgentDetectAnomalousRequestRate::GetActionClass() const
{
  return SecurityKeynodes::action_detect_anomalous_request_rate;
}

ScResult AgentDetectAnomalousRequestRate::DoProgram(ScAction & action)
{
  auto const & [rateLinkAddr, baselineLinkAddr, kSigmaLinkAddr, resultLevelAddr] = action.GetArguments<4>();
  if (!m_context.IsElement(rateLinkAddr) || !m_context.IsElement(baselineLinkAddr)
      || !m_context.IsElement(kSigmaLinkAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentDetectAnomalousRequestRate: action does not have all four arguments.");
    return action.FinishWithError();
  }

  float rate = 0.0f;
  float baseline = 0.0f;
  float kSigma = 0.0f;
  if (!ReadFloatLinkContent(m_context, rateLinkAddr, rate) || !ReadFloatLinkContent(m_context, baselineLinkAddr, baseline)
      || !ReadFloatLinkContent(m_context, kSigmaLinkAddr, kSigma))
  {
    m_logger.Error("AgentDetectAnomalousRequestRate: one of the numeric arguments is not a numeric sc-link.");
    return action.FinishWithError();
  }

  float const ratio = baseline > 0.0f ? rate / baseline : (rate > 0.0f ? std::numeric_limits<float>::infinity() : 0.0f);
  bool const anomalous = ratio > kSigma;

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
  WriteFloatLinkContent(m_context, valueLinkAddr, anomalous ? 1.0f : 0.0f);

  if (anomalous)
  {
    ScAddr const incidentAddr = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(ScType::ConstPermPosArc, SecurityKeynodes::concept_security_incident, incidentAddr);
    m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, incidentAddr);
  }

  action.FormResult(resultLevelAddr, valueLinkAddr);
  m_logger.Debug(
      "AgentDetectAnomalousRequestRate: rate=", rate, " baseline=", baseline, " k_sigma=", kSigma, " anomalous=", anomalous);

  return action.FinishSuccessfully();
}
