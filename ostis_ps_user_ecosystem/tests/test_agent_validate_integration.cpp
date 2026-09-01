#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_validate_integration.hpp"
#include "keynodes/integration_keynodes.hpp"

using AgentValidateIntegrationTest = ScMemoryTest;

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

// explained_by(ir, e) есть, conflicts_with(ir, _) нет -> valid = 1.0.
TEST_F(AgentValidateIntegrationTest, ValidatesSuccessfullyWhenExplainedAndConflictFree)
{
  m_ctx->SubscribeAgent<AgentValidateIntegration>();

  ScAddr const integratedResultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const explanationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, integratedResultAddr, explanationAddr, IntegrationKeynodes::nrel_explained_by);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(IntegrationKeynodes::action_validate_integration);
  action.SetArguments(integratedResultAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentValidateIntegration>();
}

// Нет explained_by -- невалидно, даже без конфликтов.
TEST_F(AgentValidateIntegrationTest, InvalidWhenNoExplanation)
{
  m_ctx->SubscribeAgent<AgentValidateIntegration>();

  ScAddr const integratedResultAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без explained_by
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(IntegrationKeynodes::action_validate_integration);
  action.SetArguments(integratedResultAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentValidateIntegration>();
}

// conflicts_with материализован только как входящая дуга (competitor -> ir),
// как в scenario_smart_city_traffic.scs -- агент обязан увидеть конфликт
// и во входящем направлении, не только в исходящем.
TEST_F(AgentValidateIntegrationTest, InvalidWhenConflictOnlyIncoming)
{
  m_ctx->SubscribeAgent<AgentValidateIntegration>();

  ScAddr const integratedResultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const explanationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, integratedResultAddr, explanationAddr, IntegrationKeynodes::nrel_explained_by);

  ScAddr const competitorAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, competitorAddr, integratedResultAddr, IntegrationKeynodes::nrel_conflicts_with);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(IntegrationKeynodes::action_validate_integration);
  action.SetArguments(integratedResultAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentValidateIntegration>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentValidateIntegrationTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentValidateIntegration>();

  ScAction action = m_ctx->GenerateAction(IntegrationKeynodes::action_validate_integration);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentValidateIntegration>();
}
