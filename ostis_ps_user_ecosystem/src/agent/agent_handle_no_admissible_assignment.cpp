#include "agent_handle_no_admissible_assignment.hpp"

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/allocation_keynodes.hpp"
#include "keynodes/integrity_keynodes.hpp"

using trust_utils::LinkByRelation;
using trust_utils::WriteFloatLinkContent;

ScAddr AgentHandleNoAdmissibleAssignment::GetActionClass() const
{
  return AllocationKeynodes::action_handle_no_admissible_assignment;
}

ScResult AgentHandleNoAdmissibleAssignment::DoProgram(ScAction & action)
{
  auto const & [subtaskAddr, resultLevelAddr] = action.GetArguments<2>();
  if (!m_context.IsElement(subtaskAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentHandleNoAdmissibleAssignment: action does not have both arguments.");
    return action.FinishWithError();
  }

  bool const decomposable = m_context.CheckConnector(
      AllocationKeynodes::concept_subtask_composite, subtaskAddr, ScType::ConstPermPosArc);

  if (!decomposable)
  {
    ScAddr const violationAddr = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(
        ScType::ConstPermPosArc, AllocationKeynodes::concept_integrity_violation, violationAddr);
    m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, violationAddr);
    LinkByRelation(m_context, violationAddr, subtaskAddr, IntegrityKeynodes::nrel_concerns);
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
  WriteFloatLinkContent(m_context, valueLinkAddr, decomposable ? 1.0f : 0.0f);

  action.FormResult(subtaskAddr, valueLinkAddr);
  m_logger.Debug("AgentHandleNoAdmissibleAssignment: decomposable = ", decomposable);

  return action.FinishSuccessfully();
}
