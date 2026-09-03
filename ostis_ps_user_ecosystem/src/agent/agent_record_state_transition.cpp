#include "agent_record_state_transition.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/lifecycle_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::LinkByRole;
using trust_utils::ReadFloatLinkContent;
using trust_utils::SetFunctionalRelationTarget;
using trust_utils::TargetOf;
using trust_utils::WriteFloatLinkContent;

namespace
{
// Класс из 4 распознаваемых в Фазе 1 (см. agent_record_state_transition.hpp),
// которому принадлежит stateAddr, либо ScAddr::Empty.
ScAddr ClassOfState(ScAgentContext & context, ScAddr const & stateAddr)
{
  ScAddr const candidates[] = {
      LifecycleKeynodes::concept_state_execution,
      LifecycleKeynodes::concept_state_assignment,
      LifecycleKeynodes::concept_state_aggregation,
      LifecycleKeynodes::concept_state_verification,
  };
  for (ScAddr const & classAddr : candidates)
  {
    if (context.CheckConnector(classAddr, stateAddr, ScType::ConstPermPosArc))
      return classAddr;
  }
  return ScAddr::Empty;
}

bool IsTransitionAdmissible(ScAgentContext & context, ScAddr const & fromClassAddr, ScAddr const & toClassAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      fromClassAddr, ScType::ConstCommonArc, toClassAddr, ScType::ConstPermPosArc,
      LifecycleKeynodes::nrel_transitions_to);
  return it5->Next();
}

bool ReadResource(ScAgentContext & context, ScAddr const & stateAddr, ScAddr const & relationAddr, float & outValue)
{
  ScAddr const linkAddr = TargetOf(context, stateAddr, relationAddr);
  return context.IsElement(linkAddr) && ReadFloatLinkContent(context, linkAddr, outValue);
}

void WriteResource(ScAgentContext & context, ScAddr const & stateAddr, ScAddr const & relationAddr, float value)
{
  ScAddr const linkAddr = context.GenerateLink(ScType::ConstNodeLink);
  WriteFloatLinkContent(context, linkAddr, value);
  LinkByRelation(context, stateAddr, linkAddr, relationAddr);
}
}  // namespace

ScAddr AgentRecordStateTransition::GetActionClass() const
{
  return LifecycleKeynodes::action_record_state_transition;
}

ScResult AgentRecordStateTransition::DoProgram(ScAction & action)
{
  auto const & [requestAddr, toClassAddr, transitionEventAddr, planVersionAddr] = action.GetArguments<4>();
  if (!m_context.IsElement(requestAddr) || !m_context.IsElement(toClassAddr)
      || !m_context.IsElement(transitionEventAddr) || !m_context.IsElement(planVersionAddr))
  {
    m_logger.Error("AgentRecordStateTransition: action does not have all four arguments.");
    return action.FinishWithError();
  }

  ScAddr const oldStateAddr = TargetOf(m_context, requestAddr, LifecycleKeynodes::nrel_has_state);
  if (!m_context.IsElement(oldStateAddr))
  {
    m_logger.Error("AgentRecordStateTransition: request has no current state.");
    return action.FinishWithError();
  }

  ScAddr const fromClassAddr = ClassOfState(m_context, oldStateAddr);
  if (!m_context.IsElement(fromClassAddr))
  {
    m_logger.Error("AgentRecordStateTransition: current state is not a Phase 1 recognized class.");
    return action.FinishWithError();
  }

  if (!IsTransitionAdmissible(m_context, fromClassAddr, toClassAddr))
  {
    m_logger.Error("AgentRecordStateTransition: transition is not admissible (no nrel_transitions_to edge).");
    return action.FinishWithError();
  }

  float bRem = 0.0f;
  float dRem = 0.0f;
  if (!ReadResource(m_context, oldStateAddr, LifecycleKeynodes::nrel_remaining_reassignment_budget, bRem)
      || !ReadResource(m_context, oldStateAddr, LifecycleKeynodes::nrel_remaining_replanning_depth, dRem))
  {
    m_logger.Error("AgentRecordStateTransition: current state is missing resource bookkeeping links.");
    return action.FinishWithError();
  }

  bool const isReassignmentEdge = fromClassAddr == LifecycleKeynodes::concept_state_execution
                                   && toClassAddr == LifecycleKeynodes::concept_state_assignment;
  if (isReassignmentEdge)
  {
    if (bRem <= 0.0f)
    {
      m_logger.Error("AgentRecordStateTransition: reassignment budget exhausted (b_rem = 0).");
      return action.FinishWithError();
    }
    bRem -= 1.0f;
  }

  ScAddr const newStateAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, toClassAddr, newStateAddr);
  WriteResource(m_context, newStateAddr, LifecycleKeynodes::nrel_remaining_reassignment_budget, bRem);
  WriteResource(m_context, newStateAddr, LifecycleKeynodes::nrel_remaining_replanning_depth, dRem);

  SetFunctionalRelationTarget(m_context, requestAddr, LifecycleKeynodes::nrel_has_state, newStateAddr);

  // Реификация перехода (constraint_transition_provenance): узел
  // classified concept_state_transition, роли from/to/event через
  // rrel_* (subject-start форма, см. scenario_lifecycle_recovery.scs).
  ScAddr const transitionAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, LifecycleKeynodes::concept_state_transition, transitionAddr);
  LinkByRole(m_context, transitionAddr, oldStateAddr, LifecycleKeynodes::rrel_from_state);
  LinkByRole(m_context, transitionAddr, newStateAddr, LifecycleKeynodes::rrel_to_state);
  LinkByRole(m_context, transitionAddr, transitionEventAddr, LifecycleKeynodes::rrel_transition_event);

  LinkByRelation(m_context, transitionAddr, transitionEventAddr, LifecycleKeynodes::nrel_caused_by_event);
  LinkByRelation(m_context, transitionAddr, planVersionAddr, LifecycleKeynodes::nrel_uses_plan_version);

  ScAddr const auditRecordAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, LifecycleKeynodes::concept_audit_record, auditRecordAddr);
  m_context.GenerateConnector(
      ScType::ConstPermPosArc, LifecycleKeynodes::concept_audit_record_transition, auditRecordAddr);
  LinkByRelation(m_context, transitionAddr, auditRecordAddr, LifecycleKeynodes::nrel_transition_recorded_in);

  ScAddr const provenanceAddr = m_context.GenerateNode(ScType::ConstNode);
  m_context.GenerateConnector(ScType::ConstPermPosArc, LifecycleKeynodes::concept_provenance_record, provenanceAddr);
  LinkByRelation(m_context, auditRecordAddr, provenanceAddr, LifecycleKeynodes::nrel_has_provenance);

  action.FormResult(newStateAddr, transitionAddr, auditRecordAddr);
  m_logger.Debug("AgentRecordStateTransition: transition recorded.");

  return action.FinishSuccessfully();
}
