#include "agent_check_acyclicity.hpp"

#include <unordered_set>

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/task_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::WriteFloatLinkContent;

namespace
{
// DFS с различением onStack (текущий путь обхода -- переход сюда
// означает цикл) и visited (все посещённые узлы -- не даёт повторно
// обходить сходящиеся, но не циклические пути DAG).
bool DfsHasCycle(
    ScAgentContext & context,
    ScAddr const & nodeAddr,
    std::unordered_set<ScAddr, ScAddrHashFunc> & onStack,
    std::unordered_set<ScAddr, ScAddrHashFunc> & visited)
{
  onStack.insert(nodeAddr);
  visited.insert(nodeAddr);

  ScIterator5Ptr const it5 = context.CreateIterator5(
      nodeAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc,
      TaskKeynodes::nrel_subtask_depends_on);
  while (it5->Next())
  {
    ScAddr const nextAddr = it5->Get(2);
    if (onStack.find(nextAddr) != onStack.end())
      return true;

    if (visited.find(nextAddr) == visited.end() && DfsHasCycle(context, nextAddr, onStack, visited))
      return true;
  }

  onStack.erase(nodeAddr);
  return false;
}

bool IsAcyclic(ScAgentContext & context, ScAddr const & requestAddr)
{
  std::unordered_set<ScAddr, ScAddrHashFunc> onStack;
  std::unordered_set<ScAddr, ScAddrHashFunc> visited;

  ScIterator5Ptr const subtaskIt = context.CreateIterator5(
      requestAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc,
      TaskKeynodes::nrel_decomposed_into);
  while (subtaskIt->Next())
  {
    ScAddr const subtaskAddr = subtaskIt->Get(2);
    if (visited.find(subtaskAddr) == visited.end() && DfsHasCycle(context, subtaskAddr, onStack, visited))
      return false;
  }
  return true;
}
}  // namespace

ScAddr AgentCheckAcyclicity::GetActionClass() const
{
  return TaskKeynodes::action_check_acyclicity;
}

ScResult AgentCheckAcyclicity::DoProgram(ScAction & action)
{
  auto const & [requestAddr, resultLevelAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(requestAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentCheckAcyclicity: action does not have both arguments.");
    return action.FinishWithError();
  }

  bool const valid = IsAcyclic(m_context, requestAddr);

  if (!valid)
  {
    ScAddr const violationAddr = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(
        ScType::ConstPermPosArc, IntegrityKeynodes::concept_acyclicity_constraint_violation, violationAddr);
    m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, violationAddr);
    LinkByRelation(m_context, violationAddr, requestAddr, IntegrityKeynodes::nrel_concerns);
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

  action.FormResult(requestAddr, valueLinkAddr);
  m_logger.Debug("AgentCheckAcyclicity: valid = ", valid);

  return action.FinishSuccessfully();
}
