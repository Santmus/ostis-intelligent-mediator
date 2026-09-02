#include "agent_link_explanation_to_provenance.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/relation_io.hpp"
#include "keynodes/explanation_keynodes.hpp"
#include "keynodes/provenance_keynodes.hpp"

using trust_utils::ForEachRoleTarget;
using trust_utils::LinkByRelation;

ScAddr AgentLinkExplanationToProvenance::GetActionClass() const
{
  return ProvenanceKeynodes::action_link_explanation_to_provenance;
}

ScResult AgentLinkExplanationToProvenance::DoProgram(ScAction & action)
{
  auto const & args = action.GetArguments<4>();
  ScAddr const constructionActAddr = std::get<0>(args);
  ScAddr const explanationEntityAddr = std::get<1>(args);
  ScAddr const explanationActivityAddr = std::get<2>(args);
  ScAddr const builderProvAgentAddr = std::get<3>(args);
  if (!m_context.IsElement(constructionActAddr) || !m_context.IsElement(explanationEntityAddr)
      || !m_context.IsElement(explanationActivityAddr) || !m_context.IsElement(builderProvAgentAddr))
  {
    m_logger.Error("AgentLinkExplanationToProvenance: action does not have all four arguments.");
    return action.FinishWithError();
  }

  ForEachRoleTarget(
      m_context, constructionActAddr, ExplanationKeynodes::rrel_uses_results,
      [&](ScAddr const & usedResultAddr)
      {
        LinkByRelation(
            m_context, explanationEntityAddr, usedResultAddr, ProvenanceKeynodes::nrel_prov_was_derived_from);
      });

  ForEachRoleTarget(
      m_context, constructionActAddr, ExplanationKeynodes::rrel_applies_rules,
      [&](ScAddr const & appliedRuleAddr)
      {
        LinkByRelation(m_context, explanationActivityAddr, appliedRuleAddr, ProvenanceKeynodes::nrel_prov_used);
      });

  LinkByRelation(
      m_context, explanationEntityAddr, builderProvAgentAddr, ProvenanceKeynodes::nrel_prov_was_attributed_to);

  action.FormResult(explanationEntityAddr, explanationActivityAddr);
  m_logger.Debug("AgentLinkExplanationToProvenance: explanation linked to provenance.");

  return action.FinishSuccessfully();
}
