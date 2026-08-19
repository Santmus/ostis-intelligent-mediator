#pragma once

#include <sc-memory/sc_agent_context.hpp>

namespace trust_utils
{
// Ищет цель некорневой (не rrel_*) ориентированной бинарной связи
// sourceAddr -[relationAddr]-> ?, представленной как ConstCommonArc,
// классифицированная относительным узлом через ConstPermPosArc --
// стандартный для sc-machine способ хранения нероле-вых бинарных
// отношений (см. ScAction::GetMaxCustomerWaitingTimeLink в sc-machine).
// Возвращает первую найденную цель либо ScAddr::Empty, если связи нет.
inline ScAddr TargetOf(ScAgentContext & context, ScAddr const & sourceAddr, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      sourceAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Создаёт связь sourceAddr -[relationAddr]-> targetAddr в том же
// представлении, которое понимает TargetOf.
inline void LinkByRelation(
    ScAgentContext & context, ScAddr const & sourceAddr, ScAddr const & targetAddr, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = context.GenerateConnector(ScType::ConstCommonArc, sourceAddr, targetAddr);
  context.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}
}  // namespace trust_utils
