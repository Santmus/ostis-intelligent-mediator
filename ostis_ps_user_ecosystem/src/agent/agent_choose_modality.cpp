#include "agent_choose_modality.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "keynodes/adaptive_interface_keynodes.hpp"

namespace
{
bool HasVisualPerceptionConstraint(ScAgentContext & context, ScAddr const & situationAddr)
{
  return context.CheckConnector(
      AdaptiveInterfaceKeynodes::concept_visual_perception_constraint, situationAddr, ScType::ConstPermPosArc);
}

bool PrefersVisualModality(ScAgentContext & context, ScAddr const & userModelAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      userModelAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      AdaptiveInterfaceKeynodes::nrel_has_preference);
  while (it5->Next())
  {
    ScAddr const preferenceAddr = it5->Get(2);
    if (context.CheckConnector(
            AdaptiveInterfaceKeynodes::concept_preference_visual_modality, preferenceAddr, ScType::ConstPermPosArc))
      return true;
  }
  return false;
}
}  // namespace

ScAddr AgentChooseModality::GetActionClass() const
{
  return AdaptiveInterfaceKeynodes::action_choose_modality;
}

ScResult AgentChooseModality::DoProgram(ScAction & action)
{
  auto const & [situationAddr, userModelAddr, resultLevelAddr] = action.GetArguments<3>();
  if (!m_context.IsElement(situationAddr) || !m_context.IsElement(userModelAddr)
      || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentChooseModality: action does not have all three arguments.");
    return action.FinishWithError();
  }

  ScAddr modalityAddr;
  if (HasVisualPerceptionConstraint(m_context, situationAddr))
    modalityAddr = AdaptiveInterfaceKeynodes::concept_modality_voice;
  else if (PrefersVisualModality(m_context, userModelAddr))
    modalityAddr = AdaptiveInterfaceKeynodes::concept_modality_visual;
  else
    modalityAddr = AdaptiveInterfaceKeynodes::concept_modality_text;

  m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, modalityAddr);

  action.FormResult(situationAddr, modalityAddr);
  m_logger.Debug("AgentChooseModality: modality chosen.");

  return action.FinishSuccessfully();
}
