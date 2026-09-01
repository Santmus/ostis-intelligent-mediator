#include "agent_validate_provenance_chain.hpp"

#include <queue>
#include <unordered_set>

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/provenance_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::WriteFloatLinkContent;

namespace
{
// Обход в ширину по nrel_derived_from от startAddr; возвращает true, как
// только достигнут узел, классифицированный concept_user_request.
// Множество посещённых узлов защищает от зацикливания -- отношение
// объявлено транзитивным, но не антирефлексивным.
bool ReachesUserRequest(ScAgentContext & context, ScAddr const & startAddr)
{
  std::unordered_set<ScAddr, ScAddrHashFunc> visited;
  std::queue<ScAddr> frontier;
  visited.insert(startAddr);
  frontier.push(startAddr);

  while (!frontier.empty())
  {
    ScAddr const currentAddr = frontier.front();
    frontier.pop();

    ScIterator5Ptr const it5 = context.CreateIterator5(
        currentAddr,
        ScType::ConstCommonArc,
        ScType::Unknown,
        ScType::ConstPermPosArc,
        ProvenanceKeynodes::nrel_derived_from);
    while (it5->Next())
    {
      ScAddr const nextAddr = it5->Get(2);
      if (context.CheckConnector(ProvenanceKeynodes::concept_user_request, nextAddr, ScType::ConstPermPosArc))
        return true;

      if (visited.insert(nextAddr).second)
        frontier.push(nextAddr);
    }
  }
  return false;
}
}  // namespace

ScAddr AgentValidateProvenanceChain::GetActionClass() const
{
  return ProvenanceKeynodes::action_validate_provenance_chain;
}

ScResult AgentValidateProvenanceChain::DoProgram(ScAction & action)
{
  auto const & [integratedResultAddr, resultLevelAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(integratedResultAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentValidateProvenanceChain: action does not have both arguments.");
    return action.FinishWithError();
  }

  bool const valid = ReachesUserRequest(m_context, integratedResultAddr);

  if (!valid)
  {
    ScAddr const violationAddr = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(
        ScType::ConstPermPosArc, IntegrityKeynodes::concept_existence_constraint_violation, violationAddr);
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
  m_logger.Debug("AgentValidateProvenanceChain: valid = ", valid);

  return action.FinishSuccessfully();
}
