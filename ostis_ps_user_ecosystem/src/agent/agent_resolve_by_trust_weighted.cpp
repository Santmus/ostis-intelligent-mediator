#include "agent_resolve_by_trust_weighted.hpp"

#include <limits>

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/integration_keynodes.hpp"

using trust_utils::ReadFloatLinkContent;

namespace
{
// Ищет цель некорневой (не rrel_*) ориентированной бинарной связи
// sourceAddr -[relationAddr]-> ?, как nrel_produced_by/nrel_has_trust_level.
ScAddr TargetOf(ScAgentContext & context, ScAddr const & sourceAddr, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      sourceAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

void LinkByRelation(
    ScAgentContext & context, ScAddr const & sourceAddr, ScAddr const & targetAddr, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = context.GenerateConnector(ScType::ConstCommonArc, sourceAddr, targetAddr);
  context.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

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
  LinkByRelation(m_context, integratedResultAddr, conflictAddr, IntegrationKeynodes::nrel_resolves_conflict);

  action.FormResult(integratedResultAddr, bestResultAddr);
  m_logger.Debug("AgentResolveByTrustWeighted: winner has trust weight ", bestWeight);

  return action.FinishSuccessfully();
}
