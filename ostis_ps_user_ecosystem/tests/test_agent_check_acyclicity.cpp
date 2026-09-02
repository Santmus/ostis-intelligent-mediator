#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_check_acyclicity.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/task_keynodes.hpp"

using AgentCheckAcyclicityTest = ScMemoryTest;

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

// Ищет среди детей resultLevelAddr узел класса
// concept_acyclicity_constraint_violation (тот же приём, что в
// test_agent_validate_provenance_chain.cpp).
ScAddr FindViolationChild(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (it3->Next())
  {
    ScAddr const childAddr = it3->Get(2);
    if (ctx.CheckConnector(
            IntegrityKeynodes::concept_acyclicity_constraint_violation, childAddr, ScType::ConstPermPosArc))
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

// request -[decomposed_into]-> s1, s2, s3; s2 depends_on s1,
// s3 depends_on s2 -- линейная цепочка без цикла.
TEST_F(AgentCheckAcyclicityTest, ValidAcyclicDependencyGraph)
{
  m_ctx->SubscribeAgent<AgentCheckAcyclicity>();

  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s2 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s3 = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, requestAddr, s1, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, requestAddr, s2, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, requestAddr, s3, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, s2, s1, TaskKeynodes::nrel_subtask_depends_on);
  LinkByRelation(*m_ctx, s3, s2, TaskKeynodes::nrel_subtask_depends_on);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_check_acyclicity);
  action.SetArguments(requestAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_FALSE(m_ctx->IsElement(FindViolationChild(*m_ctx, resultLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentCheckAcyclicity>();
}

// s1 depends_on s2, s2 depends_on s1 -- цикл; невалидно, создаётся
// нарушение, связанное с request через nrel_concerns.
TEST_F(AgentCheckAcyclicityTest, InvalidWhenDependencyGraphHasCycle)
{
  m_ctx->SubscribeAgent<AgentCheckAcyclicity>();

  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s2 = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, requestAddr, s1, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, requestAddr, s2, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, s1, s2, TaskKeynodes::nrel_subtask_depends_on);
  LinkByRelation(*m_ctx, s2, s1, TaskKeynodes::nrel_subtask_depends_on);  // замыкает цикл

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_check_acyclicity);
  action.SetArguments(requestAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));  // не зависает -- само по себе часть проверки
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  ScAddr const violationAddr = FindViolationChild(*m_ctx, resultLevelAddr);
  ASSERT_TRUE(m_ctx->IsElement(violationAddr));
  EXPECT_EQ(TargetOf(*m_ctx, violationAddr, IntegrityKeynodes::nrel_concerns), requestAddr);

  m_ctx->UnsubscribeAgent<AgentCheckAcyclicity>();
}

// request без единой decomposed_into -- T(q) пусто, проверка
// вырожденно проходит (нет узлов -- нет цикла).
TEST_F(AgentCheckAcyclicityTest, ValidWhenRequestHasNoSubtasksAtAll)
{
  m_ctx->SubscribeAgent<AgentCheckAcyclicity>();

  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без decomposed_into
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_check_acyclicity);
  action.SetArguments(requestAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentCheckAcyclicity>();
}

// s2 и s3 оба depends_on s1 (сходятся, но не образуют цикл) -- ромбовидный
// DAG не должен ложно распознаваться как цикл через повторное посещение
// s1 (различение visited/onStack).
TEST_F(AgentCheckAcyclicityTest, HandlesDiamondDependencyWithoutFalsePositive)
{
  m_ctx->SubscribeAgent<AgentCheckAcyclicity>();

  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s2 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s3 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const s4 = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, requestAddr, s1, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, requestAddr, s2, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, requestAddr, s3, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, requestAddr, s4, TaskKeynodes::nrel_decomposed_into);
  LinkByRelation(*m_ctx, s2, s1, TaskKeynodes::nrel_subtask_depends_on);
  LinkByRelation(*m_ctx, s3, s1, TaskKeynodes::nrel_subtask_depends_on);
  LinkByRelation(*m_ctx, s4, s2, TaskKeynodes::nrel_subtask_depends_on);
  LinkByRelation(*m_ctx, s4, s3, TaskKeynodes::nrel_subtask_depends_on);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_check_acyclicity);
  action.SetArguments(requestAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_FALSE(m_ctx->IsElement(FindViolationChild(*m_ctx, resultLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentCheckAcyclicity>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentCheckAcyclicityTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentCheckAcyclicity>();

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_check_acyclicity);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentCheckAcyclicity>();
}
