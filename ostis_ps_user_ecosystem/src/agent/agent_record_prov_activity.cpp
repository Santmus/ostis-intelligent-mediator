#include "agent_record_prov_activity.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/relation_io.hpp"
#include "keynodes/provenance_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::LinkByRole;

ScAddr AgentRecordProvActivity::GetActionClass() const
{
  return ProvenanceKeynodes::action_record_prov_activity;
}

ScResult AgentRecordProvActivity::DoProgram(ScAction & action)
{
  auto const & [actorAddr, inputAddr, provAgentAddr, timestampAddr] = action.GetArguments<4>();
  if (!m_context.IsElement(actorAddr) || !m_context.IsElement(inputAddr) || !m_context.IsElement(provAgentAddr))
  {
    m_logger.Error("AgentRecordProvActivity: action does not have all three required arguments.");
    return action.FinishWithError();
  }

  ScAddr const activityAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, ProvenanceKeynodes::concept_prov_activity, activityAddr);

  LinkByRole(m_context, activityAddr, actorAddr, ProvenanceKeynodes::rrel_actor);
  LinkByRole(m_context, activityAddr, inputAddr, ProvenanceKeynodes::rrel_input);
  LinkByRelation(m_context, activityAddr, inputAddr, ProvenanceKeynodes::nrel_prov_used);
  LinkByRelation(m_context, activityAddr, provAgentAddr, ProvenanceKeynodes::nrel_prov_was_associated_with);

  if (m_context.IsElement(timestampAddr))
    LinkByRole(m_context, activityAddr, timestampAddr, ProvenanceKeynodes::rrel_timestamp);

  action.FormResult(activityAddr, inputAddr);
  m_logger.Debug("AgentRecordProvActivity: PROV activity recorded.");

  return action.FinishSuccessfully();
}
