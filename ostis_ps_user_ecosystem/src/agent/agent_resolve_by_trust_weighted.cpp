#include "agent_resolve_by_trust_weighted.hpp"

#include <limits>

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/integration_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::ReadFloatLinkContent;
using trust_utils::SetFunctionalRelationTarget;
using trust_utils::TargetOf;

namespace
{
float TrustWeightOf(ScAgentContext & context, ScAddr const & resultAddr)
{
  ScAddr const systemAddr = TargetOf(context, resultAddr, IntegrationKeynodes::nrel_produced_by);
  if (!context.IsElement(systemAddr))
    return 0.0f;

  ScAddr const trustLevelAddr = TargetOf(context, systemAddr, IntegrationKeynodes::nrel_has_trust_level);
  if (!context.IsElement(trustLevelAddr))
    return 0.0f;

  ScIterator3Ptr const valueIt =
      context.CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (!valueIt->Next())
    return 0.0f;

  float weight = 0.0f;
  ReadFloatLinkContent(context, valueIt->Get(2), weight);
  return weight;
}
}  // namespace

ScAddr AgentResolveByTrustWeighted::GetActionClass() const
{
  return IntegrationKeynodes::action_resolve_by_trust_weighted;
}

ScResult AgentResolveByTrustWeighted::DoProgram(ScAction & action)
{
  auto const & [candidatesGroupAddr, conflictAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(candidatesGroupAddr) || !m_context.IsElement(conflictAddr))
  {
    m_logger.Error("AgentResolveByTrustWeighted: action does not have both arguments.");
    return action.FinishWithError();
  }

  ScAddr bestResultAddr;
  float bestWeight = -std::numeric_limits<float>::infinity();

  ScIterator3Ptr const candidatesIt =
      m_context.CreateIterator3(candidatesGroupAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (candidatesIt->Next())
  {
    ScAddr const candidateAddr = candidatesIt->Get(2);
    float const weight = TrustWeightOf(m_context, candidateAddr);
    if (weight > bestWeight)
    {
      bestWeight = weight;
      bestResultAddr = candidateAddr;
    }
  }

  if (!m_context.IsElement(bestResultAddr))
  {
    m_logger.Error("AgentResolveByTrustWeighted: candidates group is empty, nothing to choose from.");
    return action.FinishWithError();
  }

  ScAddr const integratedResultAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, IntegrationKeynodes::concept_integrated_result, integratedResultAddr);

  LinkByRelation(m_context, bestResultAddr, integratedResultAddr, IntegrationKeynodes::nrel_integrated_in);

  // Выбор по доверию -- одна из стратегий разрешения конфликта, а не
  // прямое основание для nrel_resolves_conflict (см. РАСХОЖДЕНИЕ С
  // РЕАЛИЗАЦИЕЙ в relations/relations_trust.scs): конфликт получает
  // овеществлённый акт разрешения и статус, а resolves_conflict
  // относится к порождённому объяснению, не к самому результату.
  ScAddr const resolutionActAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, IntegrationKeynodes::concept_conflict_resolution_act, resolutionActAddr);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, IntegrationKeynodes::concept_resolution_by_trust_weighting, resolutionActAddr);

  LinkByRelation(m_context, conflictAddr, resolutionActAddr, IntegrationKeynodes::nrel_resolved_by);
  SetFunctionalRelationTarget(
      m_context, conflictAddr, IntegrationKeynodes::nrel_conflict_status,
      IntegrationKeynodes::concept_conflict_status_resolved);

  ScAddr const explanationAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, IntegrationKeynodes::concept_explanation, explanationAddr);
  LinkByRelation(m_context, explanationAddr, conflictAddr, IntegrationKeynodes::nrel_resolves_conflict);
  LinkByRelation(m_context, integratedResultAddr, explanationAddr, IntegrationKeynodes::nrel_explained_by);

  action.FormResult(integratedResultAddr, bestResultAddr, resolutionActAddr, explanationAddr);
  m_logger.Debug("AgentResolveByTrustWeighted: winner has trust weight ", bestWeight);

  return action.FinishSuccessfully();
}
