#include "agent_evaluate_progressive_disclosure.hpp"

#include <unordered_map>
#include <unordered_set>

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "keynodes/adaptive_interface_keynodes.hpp"
#include "keynodes/provenance_keynodes.hpp"

using trust_utils::WriteFloatLinkContent;

namespace
{
// Глубина самой длинной цепочки исходящих дуг nrel_derived_from от
// nodeAddr; мемоизирована (depthCache), защищена от цикла через onStack
// -- узел, уже находящийся на текущем пути обхода, не обходится
// повторно (вносит 0 в накопленную глубину этой ветки, а не зависает).
size_t ProvenanceDepth(
    ScAgentContext & context,
    ScAddr const & nodeAddr,
    std::unordered_set<ScAddr, ScAddrHashFunc> & onStack,
    std::unordered_map<ScAddr, size_t, ScAddrHashFunc> & depthCache)
{
  auto const cached = depthCache.find(nodeAddr);
  if (cached != depthCache.end())
    return cached->second;

  if (onStack.find(nodeAddr) != onStack.end())
    return 0;

  onStack.insert(nodeAddr);

  size_t maxDepth = 0;
  ScIterator5Ptr const it5 = context.CreateIterator5(
      nodeAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      ProvenanceKeynodes::nrel_derived_from);
  while (it5->Next())
  {
    ScAddr const nextAddr = it5->Get(2);
    size_t const childDepth = 1 + ProvenanceDepth(context, nextAddr, onStack, depthCache);
    if (childDepth > maxDepth)
      maxDepth = childDepth;
  }

  onStack.erase(nodeAddr);
  depthCache[nodeAddr] = maxDepth;
  return maxDepth;
}

size_t CountGroupMembers(ScAgentContext & context, ScAddr const & groupAddr)
{
  size_t count = 0;
  ScIterator3Ptr const membersIt = context.CreateIterator3(groupAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (membersIt->Next())
    ++count;
  return count;
}
}  // namespace

ScAddr AgentEvaluateProgressiveDisclosure::GetActionClass() const
{
  return AdaptiveInterfaceKeynodes::action_evaluate_progressive_disclosure;
}

ScResult AgentEvaluateProgressiveDisclosure::DoProgram(ScAction & action)
{
  auto const & [explanationAddr, partialResultsGroupAddr, resultLevelAddr] = action.GetArguments<3>();
  if (!m_context.IsElement(explanationAddr) || !m_context.IsElement(partialResultsGroupAddr)
      || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentEvaluateProgressiveDisclosure: action does not have all three arguments.");
    return action.FinishWithError();
  }

  std::unordered_set<ScAddr, ScAddrHashFunc> onStack;
  std::unordered_map<ScAddr, size_t, ScAddrHashFunc> depthCache;
  size_t const depth = ProvenanceDepth(m_context, explanationAddr, onStack, depthCache);
  size_t const partialResultCount = CountGroupMembers(m_context, partialResultsGroupAddr);

  bool const needsProgressiveDisclosure = depth > 4 || partialResultCount > 5;

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
  WriteFloatLinkContent(m_context, valueLinkAddr, needsProgressiveDisclosure ? 1.0f : 0.0f);

  action.FormResult(explanationAddr, valueLinkAddr);
  m_logger.Debug("AgentEvaluateProgressiveDisclosure: needsProgressiveDisclosure = ", needsProgressiveDisclosure);

  return action.FinishSuccessfully();
}
