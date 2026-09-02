#include "agent_evaluate_context_aware_layout.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/adaptive_interface_keynodes.hpp"

using trust_utils::WriteFloatLinkContent;

namespace
{
bool HasLowVisibilityIndicator(ScAgentContext & context, ScAddr const & situationAddr)
{
  return context.CheckConnector(
             AdaptiveInterfaceKeynodes::concept_night_shift_context, situationAddr, ScType::ConstPermPosArc)
      || context.CheckConnector(
             AdaptiveInterfaceKeynodes::concept_low_light_context, situationAddr, ScType::ConstPermPosArc);
}
}  // namespace

ScAddr AgentEvaluateContextAwareLayout::GetActionClass() const
{
  return AdaptiveInterfaceKeynodes::action_evaluate_context_aware_layout;
}

ScResult AgentEvaluateContextAwareLayout::DoProgram(ScAction & action)
{
  auto const & [situationAddr, resultLevelAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(situationAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentEvaluateContextAwareLayout: action does not have both arguments.");
    return action.FinishWithError();
  }

  bool const triggered = HasLowVisibilityIndicator(m_context, situationAddr);

  if (triggered)
  {
    m_context.GenerateConnector(
        ScType::ConstPermPosArc, resultLevelAddr, AdaptiveInterfaceKeynodes::concept_strategy_context_aware_layout);
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
  WriteFloatLinkContent(m_context, valueLinkAddr, triggered ? 1.0f : 0.0f);

  action.FormResult(situationAddr, valueLinkAddr);
  m_logger.Debug("AgentEvaluateContextAwareLayout: triggered = ", triggered);

  return action.FinishSuccessfully();
}
