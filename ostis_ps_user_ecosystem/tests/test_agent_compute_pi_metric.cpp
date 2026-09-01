#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_compute_pi_metric.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/provenance_keynodes.hpp"

using AgentComputePiMetricTest = ScMemoryTest;

namespace
{
ScAddr MakeElementWithProvenance(ScAgentContext & ctx)
{
  ScAddr const elementAddr = ctx.GenerateNode(ScType::ConstNode);
  ScAddr const recordAddr = ctx.GenerateNode(ScType::ConstNode);
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, elementAddr, recordAddr);
  ctx.GenerateConnector(ScType::ConstPermPosArc, ProvenanceKeynodes::nrel_has_provenance, arcAddr);
  return elementAddr;
}

// Ищет среди детей resultLevelAddr узел класса
// concept_existence_constraint_violation (тот же приём, что и в
// test_agent_validate_cardinality_constraint.cpp). Возвращает
// ScAddr::Empty, если такого нарушения нет.
ScAddr FindViolationChild(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (it3->Next())
  {
    ScAddr const childAddr = it3->Get(2);
    if (ctx.CheckConnector(
            IntegrityKeynodes::concept_existence_constraint_violation, childAddr, ScType::ConstPermPosArc))
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

// Воспроизводит иллюстративный пример из scenario_explanation_construction.scs:
// pi_dec = 5/5 = 1.0 (все акты принятия решений зафиксированы провенансом).
TEST_F(AgentComputePiMetricTest, ReproducesScenarioAllFiveHaveProvenance)
{
  m_ctx->SubscribeAgent<AgentComputePiMetric>();

  ScAddr const groupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  for (int i = 0; i < 5; ++i)
  {
    ScAddr const elementAddr = MakeElementWithProvenance(*m_ctx);
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, groupAddr, elementAddr);
  }

  ScAddr const metricLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_compute_pi_metric);
  action.SetArguments(groupAddr, metricLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const it3 =
      m_ctx->CreateIterator3(metricLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(it3->Next());

  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(it3->Get(2), content));
  EXPECT_NEAR(std::stof(content), 1.0f, 0.0001f);

  // pi = 1.0 -- полное покрытие, нарушение не создаётся.
  EXPECT_FALSE(m_ctx->IsElement(FindViolationChild(*m_ctx, metricLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentComputePiMetric>();
}

// Частичное покрытие провенансом: 3 из 5 элементов имеют провенанс -> 0.6.
// Проверяет, что подсчёт не тривиален (не всегда 0 или 1), и что
// pi < 1.0 при непустой группе создаёт IntegrityViolation, связанное
// с проверяемой группой через nrel_concerns (AGENTS_AUDIT.md п.6.7).
TEST_F(AgentComputePiMetricTest, ComputesPartialCoverageRatio)
{
  m_ctx->SubscribeAgent<AgentComputePiMetric>();

  ScAddr const groupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  for (int i = 0; i < 3; ++i)
  {
    ScAddr const elementAddr = MakeElementWithProvenance(*m_ctx);
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, groupAddr, elementAddr);
  }
  for (int i = 0; i < 2; ++i)
  {
    ScAddr const elementAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без provenance
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, groupAddr, elementAddr);
  }

  ScAddr const metricLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_compute_pi_metric);
  action.SetArguments(groupAddr, metricLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const it3 =
      m_ctx->CreateIterator3(metricLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(it3->Next());

  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(it3->Get(2), content));
  EXPECT_NEAR(std::stof(content), 0.6f, 0.0001f);

  ScAddr const violationAddr = FindViolationChild(*m_ctx, metricLevelAddr);
  ASSERT_TRUE(m_ctx->IsElement(violationAddr));
  EXPECT_EQ(TargetOf(*m_ctx, violationAddr, IntegrityKeynodes::nrel_concerns), groupAddr);

  m_ctx->UnsubscribeAgent<AgentComputePiMetric>();
}

// Пустая группа -- pi = 0.0 по соглашению, но нарушение не создаётся:
// измерять нечего, а не "всё потеряно".
TEST_F(AgentComputePiMetricTest, DoesNotCreateViolationForEmptyGroup)
{
  m_ctx->SubscribeAgent<AgentComputePiMetric>();

  ScAddr const groupAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без элементов
  ScAddr const metricLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_compute_pi_metric);
  action.SetArguments(groupAddr, metricLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_FALSE(m_ctx->IsElement(FindViolationChild(*m_ctx, metricLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentComputePiMetric>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentComputePiMetricTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentComputePiMetric>();

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_compute_pi_metric);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentComputePiMetric>();
}
