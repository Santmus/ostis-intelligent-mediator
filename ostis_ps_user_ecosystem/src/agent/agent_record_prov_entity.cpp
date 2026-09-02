#include "agent_record_prov_entity.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/relation_io.hpp"
#include "keynodes/provenance_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::SetFunctionalRelationTarget;

ScAddr AgentRecordProvEntity::GetActionClass() const
{
  return ProvenanceKeynodes::action_record_prov_entity;
}

ScResult AgentRecordProvEntity::DoProgram(ScAction & action)
{
  auto const & [resultAddr, activityAddr, agentAddr] = action.GetArguments<3>();
  if (!m_context.IsElement(resultAddr) || !m_context.IsElement(activityAddr) || !m_context.IsElement(agentAddr))
  {
    m_logger.Error("AgentRecordProvEntity: action does not have all three arguments.");
    return action.FinishWithError();
  }

  ScAddr const entityAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, ProvenanceKeynodes::concept_prov_entity, entityAddr);

  LinkByRelation(m_context, entityAddr, activityAddr, ProvenanceKeynodes::nrel_prov_was_generated_by);
  LinkByRelation(m_context, entityAddr, agentAddr, ProvenanceKeynodes::nrel_prov_was_attributed_to);
  SetFunctionalRelationTarget(m_context, resultAddr, ProvenanceKeynodes::nrel_has_provenance, entityAddr);

  action.FormResult(resultAddr, entityAddr);
  m_logger.Debug("AgentRecordProvEntity: PROV entity recorded.");

  return action.FinishSuccessfully();
}
