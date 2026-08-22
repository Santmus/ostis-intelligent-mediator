#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_register_partial_result.hpp"
#include "keynodes/task_keynodes.hpp"

using AgentRegisterPartialResultTest = ScMemoryTest;

namespace
{
void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, trg);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
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

// Стандартный путь: система прислала результат по подзадаче -- создаётся
// провенансный триплет produces/caused_by_subtask/generated_in_context,
// результат помечается concept_partial_result, статус подзадачи --
// completed.
TEST_F(AgentRegisterPartialResultTest, RegistersPartialResultAndCompletesSubtask)
{
  m_ctx->SubscribeAgent<AgentRegisterPartialResult>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const contextAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_in_progress, TaskKeynodes::nrel_subtask_status);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_register_partial_result);
  action.SetArguments(systemAddr, resultAddr, subtaskAddr, contextAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  // результат классифицирован как частичный
  EXPECT_TRUE(m_ctx->CreateIterator3(TaskKeynodes::concept_partial_result, ScType::ConstPermPosArc, resultAddr)
                  ->Next());

  // produces(system, result), caused_by_subtask(result, subtask), generated_in_context(result, context)
  EXPECT_EQ(TargetOf(*m_ctx, systemAddr, TaskKeynodes::nrel_produces), resultAddr);
  EXPECT_EQ(TargetOf(*m_ctx, resultAddr, TaskKeynodes::nrel_caused_by_subtask), subtaskAddr);
  EXPECT_EQ(TargetOf(*m_ctx, resultAddr, TaskKeynodes::nrel_generated_in_context), contextAddr);

  // статус сменился на completed (и остался единственным -- отношение функционально)
  ScIterator5Ptr const statusIt = m_ctx->CreateIterator5(
      subtaskAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, TaskKeynodes::nrel_subtask_status);
  ASSERT_TRUE(statusIt->Next());
  EXPECT_EQ(statusIt->Get(2), TaskKeynodes::concept_subtask_status_completed);
  EXPECT_FALSE(statusIt->Next());

  m_ctx->UnsubscribeAgent<AgentRegisterPartialResult>();
}

// Подзадача изначально вообще без статуса -- агент всё равно
// безусловно устанавливает completed (нет условия готовности, только
// проверка наличия аргументов).
TEST_F(AgentRegisterPartialResultTest, SetsCompletedEvenWithoutPriorStatus)
{
  m_ctx->SubscribeAgent<AgentRegisterPartialResult>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const contextAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_register_partial_result);
  action.SetArguments(systemAddr, resultAddr, subtaskAddr, contextAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_EQ(TargetOf(*m_ctx, subtaskAddr, TaskKeynodes::nrel_subtask_status), TaskKeynodes::concept_subtask_status_completed);

  m_ctx->UnsubscribeAgent<AgentRegisterPartialResult>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentRegisterPartialResultTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentRegisterPartialResult>();

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_register_partial_result);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentRegisterPartialResult>();
}
