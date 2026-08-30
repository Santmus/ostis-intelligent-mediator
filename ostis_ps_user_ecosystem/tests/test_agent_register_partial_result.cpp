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

// Стандартный путь: назначенный исполнитель прислал результат по
// подзадаче в статусе in_progress -- создаётся провенансный триплет
// produces/caused_by_subtask/generated_in_context, результат помечается
// concept_partial_result/concept_processing_result, дополнительно
// порождаются акт исполнения, запись аудита и провенансные записи для
// результата и записи аудита (AGENTS_AUDIT.md п.6.2), статус подзадачи
// -- completed.
TEST_F(AgentRegisterPartialResultTest, RegistersPartialResultAndCompletesSubtask)
{
  m_ctx->SubscribeAgent<AgentRegisterPartialResult>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const contextAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, TaskKeynodes::nrel_assigned_to);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_in_progress, TaskKeynodes::nrel_subtask_status);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_register_partial_result);
  action.SetArguments(systemAddr, resultAddr, subtaskAddr, contextAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  // результат классифицирован как частичный и как результат обработки
  EXPECT_TRUE(m_ctx->CreateIterator3(TaskKeynodes::concept_partial_result, ScType::ConstPermPosArc, resultAddr)
                  ->Next());
  EXPECT_TRUE(m_ctx->CreateIterator3(TaskKeynodes::concept_processing_result, ScType::ConstPermPosArc, resultAddr)
                  ->Next());

  // produces(system, result), caused_by_subtask(result, subtask), generated_in_context(result, context)
  EXPECT_EQ(TargetOf(*m_ctx, systemAddr, TaskKeynodes::nrel_produces), resultAddr);
  EXPECT_EQ(TargetOf(*m_ctx, resultAddr, TaskKeynodes::nrel_caused_by_subtask), subtaskAddr);
  EXPECT_EQ(TargetOf(*m_ctx, resultAddr, TaskKeynodes::nrel_generated_in_context), contextAddr);

  // акт исполнения: executes(a, subtask), результат порождён им (generated_by)
  ScAddr const executionActAddr = TargetOf(*m_ctx, resultAddr, TaskKeynodes::nrel_generated_by);
  ASSERT_TRUE(m_ctx->IsElement(executionActAddr));
  EXPECT_TRUE(
      m_ctx->CreateIterator3(TaskKeynodes::concept_execution_act, ScType::ConstPermPosArc, executionActAddr)
          ->Next());
  EXPECT_EQ(TargetOf(*m_ctx, executionActAddr, TaskKeynodes::nrel_executes), subtaskAddr);

  // запись аудита: recorded_in(a, audit_record)
  ScAddr const auditRecordAddr = TargetOf(*m_ctx, executionActAddr, TaskKeynodes::nrel_recorded_in);
  ASSERT_TRUE(m_ctx->IsElement(auditRecordAddr));
  EXPECT_TRUE(
      m_ctx->CreateIterator3(TaskKeynodes::concept_audit_record, ScType::ConstPermPosArc, auditRecordAddr)->Next());

  // провенансные записи есть и у результата, и у записи аудита (функциональное отношение)
  ScAddr const resultProvenanceAddr = TargetOf(*m_ctx, resultAddr, TaskKeynodes::nrel_has_provenance);
  ScAddr const auditProvenanceAddr = TargetOf(*m_ctx, auditRecordAddr, TaskKeynodes::nrel_has_provenance);
  EXPECT_TRUE(m_ctx->IsElement(resultProvenanceAddr));
  EXPECT_TRUE(m_ctx->IsElement(auditProvenanceAddr));
  EXPECT_NE(resultProvenanceAddr, auditProvenanceAddr);

  // статус сменился на completed (и остался единственным -- отношение функционально)
  ScIterator5Ptr const statusIt = m_ctx->CreateIterator5(
      subtaskAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, TaskKeynodes::nrel_subtask_status);
  ASSERT_TRUE(statusIt->Next());
  EXPECT_EQ(statusIt->Get(2), TaskKeynodes::concept_subtask_status_completed);
  EXPECT_FALSE(statusIt->Next());

  m_ctx->UnsubscribeAgent<AgentRegisterPartialResult>();
}

// Система, приславшая результат, не назначена исполнителем подзадачи --
// ошибка, статус подзадачи не меняется.
TEST_F(AgentRegisterPartialResultTest, FinishesWithErrorWhenSystemIsNotAssignedExecutor)
{
  m_ctx->SubscribeAgent<AgentRegisterPartialResult>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const otherSystemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const contextAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, otherSystemAddr, TaskKeynodes::nrel_assigned_to);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_in_progress, TaskKeynodes::nrel_subtask_status);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_register_partial_result);
  action.SetArguments(systemAddr, resultAddr, subtaskAddr, contextAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  EXPECT_EQ(
      TargetOf(*m_ctx, subtaskAddr, TaskKeynodes::nrel_subtask_status),
      TaskKeynodes::concept_subtask_status_in_progress);

  m_ctx->UnsubscribeAgent<AgentRegisterPartialResult>();
}

// Подзадача назначена этой системе, но ещё не in_progress (например,
// только proposed) -- ошибка: результат нельзя завершить раньше начала
// исполнения.
TEST_F(AgentRegisterPartialResultTest, FinishesWithErrorWhenSubtaskNotInProgress)
{
  m_ctx->SubscribeAgent<AgentRegisterPartialResult>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const contextAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, TaskKeynodes::nrel_assigned_to);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_proposed, TaskKeynodes::nrel_subtask_status);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_register_partial_result);
  action.SetArguments(systemAddr, resultAddr, subtaskAddr, contextAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

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
