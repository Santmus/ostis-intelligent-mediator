#include "agent_aggregate_trust.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/trust_keynodes.hpp"

using trust_utils::ReadFloatLinkContent;
using trust_utils::WriteFloatLinkContent;

namespace
{
float ReadTrustComponent(ScAgentContext & context, ScAddr const & trustLevelAddr)
{
  ScIterator3Ptr const it3 =
      context.CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  float value = 0.0f;
  if (it3->Next())
    ReadFloatLinkContent(context, it3->Get(2), value);

  return value;
}
}  // namespace

float const AgentAggregateTrust::ms_lambdaRel = 0.40f;
float const AgentAggregateTrust::ms_lambdaAcc = 0.25f;
float const AgentAggregateTrust::ms_lambdaSec = 0.15f;
float const AgentAggregateTrust::ms_lambdaTr = 0.20f;

ScAddr AgentAggregateTrust::GetActionClass() const
{
  return TrustKeynodes::action_aggregate_trust;
}

ScResult AgentAggregateTrust::DoProgram(ScAction & action)
{
  auto const & [relLevelAddr, accLevelAddr, secLevelAddr, trLevelAddr, trustLevelAddr] = action.GetArguments<5>();
  if (!m_context.IsElement(relLevelAddr) || !m_context.IsElement(accLevelAddr)
      || !m_context.IsElement(secLevelAddr) || !m_context.IsElement(trLevelAddr)
      || !m_context.IsElement(trustLevelAddr))
  {
    m_logger.Error("AgentAggregateTrust: action does not have all five arguments.");
    return action.FinishWithError();
  }

  float const tRel = ReadTrustComponent(m_context, relLevelAddr);
  float const tAcc = ReadTrustComponent(m_context, accLevelAddr);
  float const tSec = ReadTrustComponent(m_context, secLevelAddr);
  float const tTr = ReadTrustComponent(m_context, trLevelAddr);

  float const trust = ms_lambdaRel * tRel + ms_lambdaAcc * tAcc + ms_lambdaSec * tSec + ms_lambdaTr * tTr;

  ScAddr valueLinkAddr;
  ScIterator3Ptr const it3 =
      m_context.CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (it3->Next())
    valueLinkAddr = it3->Get(2);
  else
  {
    valueLinkAddr = m_context.GenerateLink(ScType::ConstNodeLink);
    m_context.GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, valueLinkAddr);
  }
  WriteFloatLinkContent(m_context, valueLinkAddr, trust);

  action.FormResult(trustLevelAddr, valueLinkAddr);
  m_logger.Debug(
      "AgentAggregateTrust: trust = ",
      ms_lambdaRel,
      "*",
      tRel,
      " + ",
      ms_lambdaAcc,
      "*",
      tAcc,
      " + ",
      ms_lambdaSec,
      "*",
      tSec,
      " + ",
      ms_lambdaTr,
      "*",
      tTr,
      " = ",
      trust);

  return action.FinishSuccessfully();
}
