#include "agent_validate_right_to_explanation.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/integration_keynodes.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/security_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::WriteFloatLinkContent;

namespace
{
bool HasDeliveredTo(ScAgentContext & context, ScAddr const & integratedResultAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      integratedResultAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      SecurityKeynodes::nrel_delivered_to);
  return it5->Next();
}

bool RequiresExplanation(ScAgentContext & context, ScAddr const & requestAddr)
{
  return context.CheckConnector(
             SecurityKeynodes::concept_request_decision_support, requestAddr, ScType::ConstPermPosArc)
      || context.CheckConnector(
             SecurityKeynodes::concept_request_recommendation, requestAddr, ScType::ConstPermPosArc);
}

bool HasExplanation(ScAgentContext & context, ScAddr const & integratedResultAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      integratedResultAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      IntegrationKeynodes::nrel_explained_by);
  return it5->Next();
}
}  // namespace

ScAddr AgentValidateRightToExplanation::GetActionClass() const
{
  return SecurityKeynodes::action_validate_right_to_explanation;
}

ScResult AgentValidateRightToExplanation::DoProgram(ScAction & action)
{
  auto const & [integratedResultAddr, requestAddr, resultLevelAddr] = action.GetArguments<3>();
  if (!m_context.IsElement(integratedResultAddr) || !m_context.IsElement(requestAddr)
      || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentValidateRightToExplanation: action does not have all three arguments.");
    return action.FinishWithError();
  }

  bool const gateApplies =
      HasDeliveredTo(m_context, integratedResultAddr) && RequiresExplanation(m_context, requestAddr);
  bool const valid = !gateApplies || HasExplanation(m_context, integratedResultAddr);

  if (!valid)
  {
    ScAddr const violationAddr = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(
        ScType::ConstPermPosArc, SecurityKeynodes::concept_integrity_violation, violationAddr);
    m_context.GenerateConnector(
        ScType::ConstPermPosArc, SecurityKeynodes::concept_property_right_to_explanation, violationAddr);
    m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, violationAddr);
    LinkByRelation(m_context, violationAddr, integratedResultAddr, IntegrityKeynodes::nrel_concerns);
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
  WriteFloatLinkContent(m_context, valueLinkAddr, valid ? 1.0f : 0.0f);

  action.FormResult(integratedResultAddr, valueLinkAddr);
  m_logger.Debug("AgentValidateRightToExplanation: valid = ", valid);

  return action.FinishSuccessfully();
}
