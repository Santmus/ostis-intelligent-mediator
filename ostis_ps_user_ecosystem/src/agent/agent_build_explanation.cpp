#include "agent_build_explanation.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/relation_io.hpp"
#include "keynodes/explanation_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::LinkByRole;

namespace
{
void LinkGroupMembersByRole(
    ScAgentContext & context, ScAddr const & actAddr, ScAddr const & groupAddr, ScAddr const & roleAddr)
{
  if (!context.IsElement(groupAddr))
    return;

  ScIterator3Ptr const membersIt = context.CreateIterator3(groupAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (membersIt->Next())
    LinkByRole(context, actAddr, membersIt->Get(2), roleAddr);
}
}  // namespace

ScAddr AgentBuildExplanation::GetActionClass() const
{
  return ExplanationKeynodes::action_build_explanation;
}

ScResult AgentBuildExplanation::DoProgram(ScAction & action)
{
  auto const & [explanandumAddr, explanationTypeAddr, usesResultsGroupAddr, appliesRulesGroupAddr,
                basedOnGroupAddr, constructionTimeLinkAddr] = action.GetArguments<6>();
  if (!m_context.IsElement(explanandumAddr) || !m_context.IsElement(explanationTypeAddr))
  {
    m_logger.Error("AgentBuildExplanation: action does not have both required arguments.");
    return action.FinishWithError();
  }

  ScAddr const explanationAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, explanationTypeAddr, explanationAddr);

  LinkByRelation(m_context, explanandumAddr, explanationAddr, ExplanationKeynodes::nrel_supported_by_explanation);
  if (m_context.CheckConnector(
          ExplanationKeynodes::concept_integrated_result, explanandumAddr, ScType::ConstPermPosArc))
    LinkByRelation(m_context, explanandumAddr, explanationAddr, ExplanationKeynodes::nrel_explained_by);

  ScAddr const constructionActAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, ExplanationKeynodes::concept_explanation_construction_act, constructionActAddr);

  LinkByRole(m_context, constructionActAddr, explanandumAddr, ExplanationKeynodes::rrel_explanandum);
  LinkByRole(m_context, constructionActAddr, explanationTypeAddr, ExplanationKeynodes::rrel_explanation_type);
  LinkGroupMembersByRole(m_context, constructionActAddr, usesResultsGroupAddr, ExplanationKeynodes::rrel_uses_results);
  LinkGroupMembersByRole(
      m_context, constructionActAddr, appliesRulesGroupAddr, ExplanationKeynodes::rrel_applies_rules);
  LinkGroupMembersByRole(m_context, constructionActAddr, basedOnGroupAddr, ExplanationKeynodes::rrel_based_on);

  if (m_context.IsElement(constructionTimeLinkAddr))
    LinkByRole(
        m_context, constructionActAddr, constructionTimeLinkAddr, ExplanationKeynodes::rrel_construction_time);

  action.FormResult(explanationAddr, constructionActAddr);
  m_logger.Debug("AgentBuildExplanation: explanation constructed.");

  return action.FinishSuccessfully();
}
