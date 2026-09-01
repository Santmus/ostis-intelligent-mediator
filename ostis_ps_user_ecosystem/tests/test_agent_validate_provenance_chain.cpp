#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_validate_provenance_chain.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/provenance_keynodes.hpp"

using AgentValidateProvenanceChainTest = ScMemoryTest;

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
// concept_existence_constraint_violation (тот же приём, что в
// test_agent_compute_pi_metric.cpp).
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

// ir -[derived_from]-> r1 -[derived_from]-> request (concept_user_request)
// -- цепочка достигает запроса за два шага.
TEST_F(AgentValidateProvenanceChainTest, ValidatesSuccessfullyWhenChainReachesUserRequest)
{
  m_ctx->SubscribeAgent<AgentValidateProvenanceChain>();

  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, ProvenanceKeynodes::concept_user_request, requestAddr);

  ScAddr const intermediateAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, intermediateAddr, requestAddr, ProvenanceKeynodes::nrel_derived_from);

  ScAddr const integratedResultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, integratedResultAddr, intermediateAddr, ProvenanceKeynodes::nrel_derived_from);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_validate_provenance_chain);
  action.SetArguments(integratedResultAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_FALSE(m_ctx->IsElement(FindViolationChild(*m_ctx, resultLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentValidateProvenanceChain>();
}

// Цепочка обрывается, не достигнув concept_user_request -- невалидно,
// создаётся нарушение, связанное с integrated_result через nrel_concerns.
TEST_F(AgentValidateProvenanceChainTest, InvalidWhenChainDoesNotReachRequest)
{
  m_ctx->SubscribeAgent<AgentValidateProvenanceChain>();

  ScAddr const intermediateAddr = m_ctx->GenerateNode(ScType::ConstNode);  // не user_request

  ScAddr const integratedResultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, integratedResultAddr, intermediateAddr, ProvenanceKeynodes::nrel_derived_from);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_validate_provenance_chain);
  action.SetArguments(integratedResultAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  ScAddr const violationAddr = FindViolationChild(*m_ctx, resultLevelAddr);
  ASSERT_TRUE(m_ctx->IsElement(violationAddr));
  EXPECT_EQ(TargetOf(*m_ctx, violationAddr, IntegrityKeynodes::nrel_concerns), integratedResultAddr);

  m_ctx->UnsubscribeAgent<AgentValidateProvenanceChain>();
}

// integrated_result без единой исходящей derived_from -- цепочки нет
// вовсе, тот же невалидный исход, что и с оборванной цепочкой.
TEST_F(AgentValidateProvenanceChainTest, InvalidWhenNoOutgoingArcsAtAll)
{
  m_ctx->SubscribeAgent<AgentValidateProvenanceChain>();

  ScAddr const integratedResultAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без derived_from
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_validate_provenance_chain);
  action.SetArguments(integratedResultAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);
  EXPECT_TRUE(m_ctx->IsElement(FindViolationChild(*m_ctx, resultLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentValidateProvenanceChain>();
}

// Цикл в графе derived_from (r1 -> r2 -> r1), не достигающий
// concept_user_request -- обход обязан завершиться (не зависнуть) и
// корректно вернуть invalid, благодаря множеству посещённых узлов.
TEST_F(AgentValidateProvenanceChainTest, HandlesCycleWithoutHangingAndReportsInvalid)
{
  m_ctx->SubscribeAgent<AgentValidateProvenanceChain>();

  ScAddr const r1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const r2 = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, r1, r2, ProvenanceKeynodes::nrel_derived_from);
  LinkByRelation(*m_ctx, r2, r1, ProvenanceKeynodes::nrel_derived_from);  // замыкает цикл

  ScAddr const integratedResultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, integratedResultAddr, r1, ProvenanceKeynodes::nrel_derived_from);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_validate_provenance_chain);
  action.SetArguments(integratedResultAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));  // не зависает -- само по себе часть проверки
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentValidateProvenanceChain>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentValidateProvenanceChainTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentValidateProvenanceChain>();

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_validate_provenance_chain);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentValidateProvenanceChain>();
}
