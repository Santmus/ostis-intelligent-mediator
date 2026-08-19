#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_dispatch_ready_subtask.hpp"
#include "keynodes/task_keynodes.hpp"

using AgentDispatchReadySubtaskTest = ScMemoryTest;

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
}  // namespace

// assigned_to + status=proposed + нет зависимостей -> подзадача готова:
// статус меняется на in_progress, создаётся act_request.
TEST_F(AgentDispatchReadySubtaskTest, DispatchesSubtaskWithNoDependencies)
{
  m_ctx->SubscribeAgent<AgentDispatchReadySubtask>();

  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, TaskKeynodes::nrel_assigned_to);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_proposed, TaskKeynodes::nrel_subtask_status);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_dispatch_ready_subtask);
  action.SetArguments(subtaskAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);

  // статус сменился на in_progress (и только он -- отношение функционально)
  ScIterator5Ptr const statusIt = m_ctx->CreateIterator5(
      subtaskAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, TaskKeynodes::nrel_subtask_status);
  ASSERT_TRUE(statusIt->Next());
  EXPECT_EQ(statusIt->Get(2), TaskKeynodes::concept_subtask_status_in_progress);
  EXPECT_FALSE(statusIt->Next());  // ровно один статус

  // создан act_request с receiver=systemAddr
  ScIterator5Ptr const actIt = m_ctx->CreateIterator5(
      ScType::ConstNode,
      ScType::ConstPermPosArc,
      systemAddr,
      ScType::ConstPermPosArc,
      TaskKeynodes::rrel_receiver);
  EXPECT_TRUE(actIt->Next());

  m_ctx->UnsubscribeAgent<AgentDispatchReadySubtask>();
}

// Есть незавершённая зависимость -- подзадача не запускается, статус
// не меняется, индикатор 0 (не ошибка).
TEST_F(AgentDispatchReadySubtaskTest, DoesNotDispatchWhenDependencyIncomplete)
{
  m_ctx->SubscribeAgent<AgentDispatchReadySubtask>();

  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, TaskKeynodes::nrel_assigned_to);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_proposed, TaskKeynodes::nrel_subtask_status);

  ScAddr const dependencyAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(
      *m_ctx, dependencyAddr, TaskKeynodes::concept_subtask_status_in_progress, TaskKeynodes::nrel_subtask_status);
  LinkByRelation(*m_ctx, subtaskAddr, dependencyAddr, TaskKeynodes::nrel_subtask_depends_on);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_dispatch_ready_subtask);
  action.SetArguments(subtaskAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  // статус остался proposed
  ScIterator5Ptr const statusIt = m_ctx->CreateIterator5(
      subtaskAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, TaskKeynodes::nrel_subtask_status);
  ASSERT_TRUE(statusIt->Next());
  EXPECT_EQ(statusIt->Get(2), TaskKeynodes::concept_subtask_status_proposed);

  m_ctx->UnsubscribeAgent<AgentDispatchReadySubtask>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentDispatchReadySubtaskTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentDispatchReadySubtask>();

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_dispatch_ready_subtask);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentDispatchReadySubtask>();
}
