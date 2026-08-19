#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_compute_pi_metric.hpp"
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

  m_ctx->UnsubscribeAgent<AgentComputePiMetric>();
}

// Частичное покрытие провенансом: 3 из 5 элементов имеют провенанс -> 0.6.
// Проверяет, что подсчёт не тривиален (не всегда 0 или 1).
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
