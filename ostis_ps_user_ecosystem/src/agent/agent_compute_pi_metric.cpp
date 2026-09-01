#include "agent_compute_pi_metric.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/provenance_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::WriteFloatLinkContent;

namespace
{
bool HasProvenance(ScAgentContext & context, ScAddr const & elementAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      elementAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      ProvenanceKeynodes::nrel_has_provenance);
  return it5->Next();
}
}  // namespace

ScAddr AgentComputePiMetric::GetActionClass() const
{
  return ProvenanceKeynodes::action_compute_pi_metric;
}

ScResult AgentComputePiMetric::DoProgram(ScAction & action)
{
  auto const & [elementsGroupAddr, metricLevelAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(elementsGroupAddr) || !m_context.IsElement(metricLevelAddr))
  {
    m_logger.Error("AgentComputePiMetric: action does not have both arguments.");
    return action.FinishWithError();
  }

  size_t total = 0;
  size_t withProvenance = 0;
  ScIterator3Ptr const membersIt = m_context.CreateIterator3(elementsGroupAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (membersIt->Next())
  {
    ++total;
    if (HasProvenance(m_context, membersIt->Get(2)))
      ++withProvenance;
  }

  float const pi = total > 0 ? static_cast<float>(withProvenance) / static_cast<float>(total) : 0.0f;

  // pi < 1.0 при непустой группе -- нарушение существования (у части
  // элементов нет провенансной записи, хотя она обязана быть). Пустая
  // группа не флагуется: измерять нечего, а не "всё потеряно" (см.
  // AGENTS_AUDIT.md п.6.7 -- вычислимая часть правила дополнена
  // созданием IntegrityViolation, восстановление пропущенного
  // провенанса по-прежнему не реализовано).
  if (total > 0 && withProvenance < total)
  {
    ScAddr const violationAddr = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(
        ScType::ConstPermPosArc, IntegrityKeynodes::concept_existence_constraint_violation, violationAddr);
    m_context.GenerateConnector(ScType::ConstPermPosArc, metricLevelAddr, violationAddr);
    LinkByRelation(m_context, violationAddr, elementsGroupAddr, IntegrityKeynodes::nrel_concerns);
  }

  ScAddr valueLinkAddr;
  ScIterator3Ptr const it3 =
      m_context.CreateIterator3(metricLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (it3->Next())
    valueLinkAddr = it3->Get(2);
  else
  {
    valueLinkAddr = m_context.GenerateLink(ScType::ConstNodeLink);
    m_context.GenerateConnector(ScType::ConstPermPosArc, metricLevelAddr, valueLinkAddr);
  }
  WriteFloatLinkContent(m_context, valueLinkAddr, pi);

  action.FormResult(metricLevelAddr, valueLinkAddr);
  m_logger.Debug("AgentComputePiMetric: pi = ", withProvenance, "/", total, " = ", pi);

  return action.FinishSuccessfully();
}
