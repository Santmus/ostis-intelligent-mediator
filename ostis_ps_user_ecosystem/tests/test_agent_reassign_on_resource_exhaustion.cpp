#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_reassign_on_resource_exhaustion.hpp"
#include "keynodes/coalition_keynodes.hpp"

using AgentReassignOnResourceExhaustionTest = ScMemoryTest;

namespace
{
void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, trg);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

float ReadIndicator(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  EXPECT_TRUE(it3->Next());
  std::string content;
  EXPECT_TRUE(ctx.GetLinkContent(it3->Get(2), content));
  return std::stof(content);
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

// Три подзадачи назначены системе -- все три получают статус
// reassigned, индикатор = 3.0.
TEST_F(AgentReassignOnResourceExhaustionTest, ReassignsAllSubtasksAssignedToSystem)
{
  m_ctx->SubscribeAgent<AgentReassignOnResourceExhaustion>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAddr const subtask1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtask2 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtask3 = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtask1, systemAddr, CoalitionKeynodes::nrel_assigned_to);
  LinkByRelation(*m_ctx, subtask2, systemAddr, CoalitionKeynodes::nrel_assigned_to);
  LinkByRelation(*m_ctx, subtask3, systemAddr, CoalitionKeynodes::nrel_assigned_to);

  ScAddr const otherSystemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const unrelatedSubtask = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, unrelatedSubtask, otherSystemAddr, CoalitionKeynodes::nrel_assigned_to);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(CoalitionKeynodes::action_reassign_on_resource_exhaustion);
  action.SetArguments(systemAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 3.0f, 0.0001f);

  EXPECT_EQ(
      TargetOf(*m_ctx, subtask1, CoalitionKeynodes::nrel_subtask_status),
      CoalitionKeynodes::concept_subtask_status_reassigned);
  EXPECT_EQ(
      TargetOf(*m_ctx, subtask2, CoalitionKeynodes::nrel_subtask_status),
      CoalitionKeynodes::concept_subtask_status_reassigned);
  EXPECT_EQ(
      TargetOf(*m_ctx, subtask3, CoalitionKeynodes::nrel_subtask_status),
      CoalitionKeynodes::concept_subtask_status_reassigned);

  // подзадача другой системы не тронута
  EXPECT_FALSE(m_ctx->IsElement(TargetOf(*m_ctx, unrelatedSubtask, CoalitionKeynodes::nrel_subtask_status)));

  m_ctx->UnsubscribeAgent<AgentReassignOnResourceExhaustion>();
}

// Уже существующий статус подзадачи корректно заменяется новым --
// nrel_subtask_status функционально, старой дуги остаться не должно.
TEST_F(AgentReassignOnResourceExhaustionTest, ReplacesExistingStatusFunctionally)
{
  m_ctx->SubscribeAgent<AgentReassignOnResourceExhaustion>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, CoalitionKeynodes::nrel_assigned_to);
  LinkByRelation(
      *m_ctx, subtaskAddr, CoalitionKeynodes::concept_subtask_status_failed, CoalitionKeynodes::nrel_subtask_status);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(CoalitionKeynodes::action_reassign_on_resource_exhaustion);
  action.SetArguments(systemAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator5Ptr const statusIt = m_ctx->CreateIterator5(
      subtaskAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc,
      CoalitionKeynodes::nrel_subtask_status);
  ASSERT_TRUE(statusIt->Next());
  EXPECT_EQ(statusIt->Get(2), CoalitionKeynodes::concept_subtask_status_reassigned);
  EXPECT_FALSE(statusIt->Next());  // ровно один статус

  m_ctx->UnsubscribeAgent<AgentReassignOnResourceExhaustion>();
}

// У системы нет назначенных подзадач -- индикатор 0.0, не ошибка.
TEST_F(AgentReassignOnResourceExhaustionTest, ReportsZeroWhenNoSubtasksAssigned)
{
  m_ctx->SubscribeAgent<AgentReassignOnResourceExhaustion>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без подзадач
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(CoalitionKeynodes::action_reassign_on_resource_exhaustion);
  action.SetArguments(systemAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentReassignOnResourceExhaustion>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentReassignOnResourceExhaustionTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentReassignOnResourceExhaustion>();

  ScAction action = m_ctx->GenerateAction(CoalitionKeynodes::action_reassign_on_resource_exhaustion);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentReassignOnResourceExhaustion>();
}
