#include "agent_validate_integration.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/integration_keynodes.hpp"

using trust_utils::WriteFloatLinkContent;

namespace
{
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

// nrel_conflicts_with объявлено симметричным, но в существующих сценариях
// KB не всегда продублировано в обе стороны -- проверяем оба направления.
bool HasConflict(ScAgentContext & context, ScAddr const & integratedResultAddr)
{
  ScIterator5Ptr const outIt = context.CreateIterator5(
      integratedResultAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      IntegrationKeynodes::nrel_conflicts_with);
  if (outIt->Next())
    return true;

  ScIterator5Ptr const inIt = context.CreateIterator5(
      ScType::Unknown,
      ScType::ConstCommonArc,
      integratedResultAddr,
      ScType::ConstPermPosArc,
      IntegrationKeynodes::nrel_conflicts_with);
  return inIt->Next();
}
}  // namespace

ScAddr AgentValidateIntegration::GetActionClass() const
{
  return IntegrationKeynodes::action_validate_integration;
}

ScResult AgentValidateIntegration::DoProgram(ScAction & action)
{
  auto const & [integratedResultAddr, resultLevelAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(integratedResultAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentValidateIntegration: action does not have both arguments.");
    return action.FinishWithError();
  }

  bool const valid = HasExplanation(m_context, integratedResultAddr) && !HasConflict(m_context, integratedResultAddr);

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
  m_logger.Debug("AgentValidateIntegration: valid = ", valid);

  return action.FinishSuccessfully();
}
