#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_handle_no_admissible_assignment.hpp"
#include "keynodes/allocation_keynodes.hpp"
#include "keynodes/integrity_keynodes.hpp"

using AgentHandleNoAdmissibleAssignmentTest = ScMemoryTest;

namespace
{
float ReadIndicator(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  EXPECT_TRUE(it3->Next());
  std::string content;
  EXPECT_TRUE(ctx.GetLinkContent(it3->Get(2), content));
  return std::stof(content);
}

// Ищет среди детей resultLevelAddr узел класса concept_integrity_violation.
ScAddr FindViolationChild(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (it3->Next())
  {
    ScAddr const childAddr = it3->Get(2);
    if (ctx.CheckConnector(AllocationKeynodes::concept_integrity_violation, childAddr, ScType::ConstPermPosArc))
      return childAddr;
  }
  return ScAddr::Empty;
}

ScAddr TargetOf(ScAgentContext & ctx, ScAddr const & src, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(src, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}
}  // namespace

// Подзадача атомарна (не composite) -- путь декомпозиции неприменим,
// создаётся нарушение целостности, связанное с подзадачей.
TEST_F(AgentHandleNoAdmissibleAssignmentTest, CreatesViolationForAtomicSubtask)
{
  m_ctx->SubscribeAgent<AgentHandleNoAdmissibleAssignment>();

  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);  // не concept_subtask_composite
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AllocationKeynodes::action_handle_no_admissible_assignment);
  action.SetArguments(subtaskAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  ScAddr const violationAddr = FindViolationChild(*m_ctx, resultLevelAddr);
  ASSERT_TRUE(m_ctx->IsElement(violationAddr));
  EXPECT_EQ(TargetOf(*m_ctx, violationAddr, IntegrityKeynodes::nrel_concerns), subtaskAddr);

  m_ctx->UnsubscribeAgent<AgentHandleNoAdmissibleAssignment>();
}

// Подзадача композитна -- путь декомпозиции применим, нарушение не
// создаётся.
TEST_F(AgentHandleNoAdmissibleAssignmentTest, DoesNotCreateViolationForCompositeSubtask)
{
  m_ctx->SubscribeAgent<AgentHandleNoAdmissibleAssignment>();

  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, AllocationKeynodes::concept_subtask_composite, subtaskAddr);
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AllocationKeynodes::action_handle_no_admissible_assignment);
  action.SetArguments(subtaskAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_FALSE(m_ctx->IsElement(FindViolationChild(*m_ctx, resultLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentHandleNoAdmissibleAssignment>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentHandleNoAdmissibleAssignmentTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentHandleNoAdmissibleAssignment>();

  ScAction action = m_ctx->GenerateAction(AllocationKeynodes::action_handle_no_admissible_assignment);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentHandleNoAdmissibleAssignment>();
}
