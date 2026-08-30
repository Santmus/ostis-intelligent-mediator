#include "agent_validate_cardinality_constraint.hpp"

#include <cmath>

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/integrity_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::ReadFloatLinkContent;
using trust_utils::WriteFloatLinkContent;

namespace
{
// Считает дуги, соответствующие одному из трёх паттернов связи,
// различаемых shape'ами только текстом (см. agent_validate_cardinality_constraint.hpp).
size_t CountArcs(
    ScAgentContext & context, ScAddr const & targetAddr, ScAddr const & modeAddr, ScAddr const & relationAddr)
{
  ScType connectorType = ScType::ConstCommonArc;
  bool targetIsSource = true;
  if (modeAddr == IntegrityKeynodes::concept_cardinality_check_outgoing_non_role)
  {
    connectorType = ScType::ConstCommonArc;
    targetIsSource = true;
  }
  else if (modeAddr == IntegrityKeynodes::concept_cardinality_check_incoming_non_role)
  {
    connectorType = ScType::ConstCommonArc;
    targetIsSource = false;
  }
  else if (modeAddr == IntegrityKeynodes::concept_cardinality_check_outgoing_role)
  {
    connectorType = ScType::ConstPermPosArc;
    targetIsSource = true;
  }
  else
    return 0;

  size_t count = 0;
  if (targetIsSource)
  {
    ScIterator5Ptr const it5 =
        context.CreateIterator5(targetAddr, connectorType, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
    while (it5->Next())
      ++count;
  }
  else
  {
    ScIterator5Ptr const it5 =
        context.CreateIterator5(ScType::Unknown, connectorType, targetAddr, ScType::ConstPermPosArc, relationAddr);
    while (it5->Next())
      ++count;
  }
  return count;
}
}  // namespace

ScAddr AgentValidateCardinalityConstraint::GetActionClass() const
{
  return IntegrityKeynodes::action_validate_cardinality_constraint;
}

ScResult AgentValidateCardinalityConstraint::DoProgram(ScAction & action)
{
  auto const & [targetAddr, relationAddr, modeAddr, minCountAddr, maxCountAddr, violationClassAddr, resultLevelAddr] =
      action.GetArguments<7>();
  if (!m_context.IsElement(targetAddr) || !m_context.IsElement(relationAddr) || !m_context.IsElement(modeAddr)
      || !m_context.IsElement(minCountAddr) || !m_context.IsElement(maxCountAddr)
      || !m_context.IsElement(violationClassAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentValidateCardinalityConstraint: action does not have all seven arguments.");
    return action.FinishWithError();
  }

  if (modeAddr != IntegrityKeynodes::concept_cardinality_check_outgoing_non_role
      && modeAddr != IntegrityKeynodes::concept_cardinality_check_incoming_non_role
      && modeAddr != IntegrityKeynodes::concept_cardinality_check_outgoing_role)
  {
    m_logger.Error("AgentValidateCardinalityConstraint: unknown mode argument.");
    return action.FinishWithError();
  }

  float minCountValue = 0.0f;
  if (!ReadFloatLinkContent(m_context, minCountAddr, minCountValue))
  {
    m_logger.Error("AgentValidateCardinalityConstraint: min_count is not a numeric sc-link.");
    return action.FinishWithError();
  }
  auto const minCount = static_cast<long>(std::lround(minCountValue));

  float maxCountValue = 0.0f;
  bool const hasMax = ReadFloatLinkContent(m_context, maxCountAddr, maxCountValue);
  auto const maxCount = hasMax ? static_cast<long>(std::lround(maxCountValue)) : 0L;

  auto const count = static_cast<long>(CountArcs(m_context, targetAddr, modeAddr, relationAddr));
  bool const valid = count >= minCount && (!hasMax || count <= maxCount);

  if (!valid)
  {
    ScAddr const violationAddr = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(ScType::ConstPermPosArc, violationClassAddr, violationAddr);
    m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, violationAddr);
    LinkByRelation(m_context, violationAddr, targetAddr, IntegrityKeynodes::nrel_concerns);
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

  action.FormResult(targetAddr, valueLinkAddr);
  m_logger.Debug("AgentValidateCardinalityConstraint: count=", count, " min=", minCount, " valid=", valid);

  return action.FinishSuccessfully();
}
