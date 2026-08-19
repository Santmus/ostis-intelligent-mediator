#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_resolve_by_trust_weighted.hpp"
#include "keynodes/integration_keynodes.hpp"
#include "keynodes/trust_keynodes.hpp"

using AgentResolveByTrustWeightedTest = ScMemoryTest;

namespace
{
void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, trg);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

// Создаёт результат, произведённый системой с заданным уровнем доверия.
ScAddr MakeResultWithTrust(ScAgentContext & ctx, float trustValue)
{
  ScAddr const resultAddr = ctx.GenerateNode(ScType::ConstNode);
  ScAddr const systemAddr = ctx.GenerateNode(ScType::ConstNode);
  LinkByRelation(ctx, resultAddr, systemAddr, IntegrationKeynodes::nrel_produced_by);

  ScAddr const trustLevelAddr = ctx.GenerateNode(ScType::ConstNode);
  ctx.GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);
  LinkByRelation(ctx, systemAddr, trustLevelAddr, IntegrationKeynodes::nrel_has_trust_level);

  ScAddr const valueLinkAddr = ctx.GenerateLink(ScType::ConstNodeLink);
  ctx.SetLinkContent(valueLinkAddr, std::to_string(trustValue));
  ctx.GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, valueLinkAddr);

  return resultAddr;
}
}  // namespace

// Три кандидата с разным доверием -- побеждает система с максимальным
// trust (0.95), а не первая/последняя по порядку добавления.
TEST_F(AgentResolveByTrustWeightedTest, PicksResultFromHighestTrustSystem)
{
  m_ctx->SubscribeAgent<AgentResolveByTrustWeighted>();

  ScAddr const groupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const resultLow = MakeResultWithTrust(*m_ctx, 0.60f);
  ScAddr const resultBest = MakeResultWithTrust(*m_ctx, 0.95f);
  ScAddr const resultMid = MakeResultWithTrust(*m_ctx, 0.80f);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, groupAddr, resultLow);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, groupAddr, resultBest);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, groupAddr, resultMid);

  ScAddr const conflictAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(IntegrationKeynodes::action_resolve_by_trust_weighted);
  action.SetArguments(groupAddr, conflictAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScStructure const resultStruct = action.GetResult();
  EXPECT_TRUE(resultStruct.Has(resultBest));
  EXPECT_FALSE(resultStruct.Has(resultLow));
  EXPECT_FALSE(resultStruct.Has(resultMid));

  ScIterator5Ptr const resolvesIt = m_ctx->CreateIterator5(
      ScType::ConstNode,
      ScType::ConstCommonArc,
      conflictAddr,
      ScType::ConstPermPosArc,
      IntegrationKeynodes::nrel_resolves_conflict);
  ASSERT_TRUE(resolvesIt->Next());
  ScAddr const integratedResultAddr = resolvesIt->Get(0);
  EXPECT_TRUE(m_ctx->CheckConnector(
      IntegrationKeynodes::concept_integrated_result, integratedResultAddr, ScType::ConstPermPosArc));

  ScIterator5Ptr const integratedInIt = m_ctx->CreateIterator5(
      resultBest,
      ScType::ConstCommonArc,
      integratedResultAddr,
      ScType::ConstPermPosArc,
      IntegrationKeynodes::nrel_integrated_in);
  EXPECT_TRUE(integratedInIt->Next());

  m_ctx->UnsubscribeAgent<AgentResolveByTrustWeighted>();
}

// Кандидат без доверительной цепочки (нет producer/trust_level) получает
// вес 0, но не считается ошибкой -- побеждает единственный кандидат
// с реальным доверием.
TEST_F(AgentResolveByTrustWeightedTest, TreatsCandidateWithoutTrustChainAsZero)
{
  m_ctx->SubscribeAgent<AgentResolveByTrustWeighted>();

  ScAddr const groupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const resultNoTrust = m_ctx->GenerateNode(ScType::ConstNode);  // без nrel_produced_by
  ScAddr const resultWithTrust = MakeResultWithTrust(*m_ctx, 0.10f);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, groupAddr, resultNoTrust);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, groupAddr, resultWithTrust);

  ScAddr const conflictAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(IntegrationKeynodes::action_resolve_by_trust_weighted);
  action.SetArguments(groupAddr, conflictAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScStructure const resultStruct = action.GetResult();
  EXPECT_TRUE(resultStruct.Has(resultWithTrust));
  EXPECT_FALSE(resultStruct.Has(resultNoTrust));

  m_ctx->UnsubscribeAgent<AgentResolveByTrustWeighted>();
}

// Пустая группа кандидатов -- ошибка (нечего выбирать), в отличие от
// метрик-агентов, где пустой вход трактуется как 0.
TEST_F(AgentResolveByTrustWeightedTest, FinishesWithErrorOnEmptyCandidatesGroup)
{
  m_ctx->SubscribeAgent<AgentResolveByTrustWeighted>();

  ScAddr const groupAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без кандидатов
  ScAddr const conflictAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(IntegrationKeynodes::action_resolve_by_trust_weighted);
  action.SetArguments(groupAddr, conflictAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentResolveByTrustWeighted>();
}
