#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_validate_cardinality_constraint.hpp"
#include "agent/agent_update_trust_reliability.hpp"
#include "agent/agent_aggregate_trust.hpp"
#include "agent/agent_resolve_by_trust_weighted.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/trust_keynodes.hpp"
#include "keynodes/integration_keynodes.hpp"

// Не отдельный агент, а сквозной тест, показывающий, что выход одного
// уже реализованного агента корректно читается следующим -- без
// оркестратора (в проекте его пока нет, см. "Сквозные зависимости" в
// agents_vault/00 Индекс.md) вызовы просто идут друг за другом внутри
// одного теста.
using IntegrationRequestPipelineTest = ScMemoryTest;

namespace
{
ScAddr MakeFloatLink(ScAgentContext & ctx, float value)
{
  ScAddr const linkAddr = ctx.GenerateLink(ScType::ConstNodeLink);
  ctx.SetLinkContent(linkAddr, std::to_string(value));
  return linkAddr;
}

void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, trg);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

float ReadValueLinkContent(ScAgentContext & ctx, ScAddr const & nodeAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(nodeAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  EXPECT_TRUE(it3->Next());
  std::string content;
  EXPECT_TRUE(ctx.GetLinkContent(it3->Get(2), content));
  return std::stof(content);
}
}  // namespace

// Воспроизводит фрагмент scenario_request_processing.scs: пользователь
// формулирует ровно один запрос (шаг 1, форма shape_user_request_authorship,
// [1,1] -- AgentValidateCardinalityConstraint); принятая с высокой оценкой
// обратная связь (шаг 8, "оценка: пользователь принял рекомендацию, 5/5")
// поднимает надёжность обработавшей запрос системы с нейтрального нуля
// (AgentUpdateTrustReliability); агрегированное доверие (AgentAggregateTrust,
// rule_aggregate_trust) используется затем, чтобы разрешить конфликт двух
// результатов (rule_resolve_by_trust_weighted) в пользу этой системы против
// заведомо менее доверенного конкурента. Цепочка: Validate -> Update ->
// Aggregate -> Resolve -- четыре разных агента, три разных семейства
// (целостность / доверие / интеграция), объединённые общими данными, а не
// новым оркестрирующим агентом.
TEST_F(IntegrationRequestPipelineTest, ValidatesUpdatesAggregatesAndResolvesConflict)
{
  m_ctx->SubscribeAgent<AgentValidateCardinalityConstraint>();
  m_ctx->SubscribeAgent<AgentUpdateTrustReliability>();
  m_ctx->SubscribeAgent<AgentAggregateTrust>();
  m_ctx->SubscribeAgent<AgentResolveByTrustWeighted>();

  // --- Шаг 1: user -[nrel_formulates]-> request, ровно один автор. ---
  ScAddr const nrelFormulatesAddr = m_ctx->GenerateNode(ScType::ConstNodeNonRole);
  ScAddr const userAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, userAddr, requestAddr, nrelFormulatesAddr);

  ScAddr const minCountAddr = MakeFloatLink(*m_ctx, 1.0f);
  ScAddr const maxCountAddr = MakeFloatLink(*m_ctx, 1.0f);
  ScAddr const validationLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction validateAction = m_ctx->GenerateAction(IntegrityKeynodes::action_validate_cardinality_constraint);
  validateAction.SetArguments(
      requestAddr,
      nrelFormulatesAddr,
      IntegrityKeynodes::concept_cardinality_check_incoming_non_role,
      minCountAddr,
      maxCountAddr,
      IntegrityKeynodes::concept_cardinality_constraint_violation,
      validationLevelAddr);
  ASSERT_TRUE(validateAction.InitiateAndWait(2000));
  ASSERT_TRUE(validateAction.IsFinishedSuccessfully());
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, validationLevelAddr), 1.0f, 0.0001f);

  // --- Шаг 2: положительный исход (nu_rel = 1.0, "оценка 5/5") поднимает
  // t_rel обработавшей запрос системы с нейтрального нуля. ---
  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const relLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, relLevelAddr);
  ScAddr const outcomeLinkAddr = MakeFloatLink(*m_ctx, 1.0f);

  ScAction updateAction = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_reliability);
  updateAction.SetArguments(relLevelAddr, outcomeLinkAddr);
  ASSERT_TRUE(updateAction.InitiateAndWait(2000));
  ASSERT_TRUE(updateAction.IsFinishedSuccessfully());
  // (1 - alpha) * 0 + alpha * 1.0, alpha = 0.10 (agent_update_trust_reliability.cpp).
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, relLevelAddr), 0.10f, 0.0001f);

  // --- Шаг 3: агрегируем -- обновлена только t_rel, остальные три
  // компоненты нейтральны (без value_link => 0). ---
  ScAddr const accLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const secLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const trLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAction aggregateAction = m_ctx->GenerateAction(TrustKeynodes::action_aggregate_trust);
  aggregateAction.SetArguments(relLevelAddr, accLevelAddr, secLevelAddr, trLevelAddr, trustLevelAddr);
  ASSERT_TRUE(aggregateAction.InitiateAndWait(2000));
  ASSERT_TRUE(aggregateAction.IsFinishedSuccessfully());
  // lambda_rel * 0.10, lambda_rel = 0.40 (agent_aggregate_trust.cpp).
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, trustLevelAddr), 0.04f, 0.0001f);

  // Замыкаем цепочку, которую ожидает AgentResolveByTrustWeighted:
  // result -[nrel_produced_by]-> system -[nrel_has_trust_level]-> trust_level.
  LinkByRelation(*m_ctx, systemAddr, trustLevelAddr, IntegrationKeynodes::nrel_has_trust_level);

  ScAddr const resultFromSystemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, resultFromSystemAddr, systemAddr, IntegrationKeynodes::nrel_produced_by);

  // --- Шаг 4: конкурирующий результат от заведомо менее доверенной
  // системы (0.01 < 0.04) -- конфликт разрешается по взвешенному доверию. ---
  ScAddr const competitorAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const competitorTrustAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, competitorTrustAddr);
  LinkByRelation(*m_ctx, competitorAddr, competitorTrustAddr, IntegrationKeynodes::nrel_has_trust_level);
  ScAddr const competitorValueLinkAddr = MakeFloatLink(*m_ctx, 0.01f);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, competitorTrustAddr, competitorValueLinkAddr);

  ScAddr const resultFromCompetitorAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, resultFromCompetitorAddr, competitorAddr, IntegrationKeynodes::nrel_produced_by);

  ScAddr const candidatesGroupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, candidatesGroupAddr, resultFromSystemAddr);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, candidatesGroupAddr, resultFromCompetitorAddr);
  ScAddr const conflictAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction resolveAction = m_ctx->GenerateAction(IntegrationKeynodes::action_resolve_by_trust_weighted);
  resolveAction.SetArguments(candidatesGroupAddr, conflictAddr);
  ASSERT_TRUE(resolveAction.InitiateAndWait(2000));
  ASSERT_TRUE(resolveAction.IsFinishedSuccessfully());

  ScStructure const resolveResult = resolveAction.GetResult();
  EXPECT_TRUE(resolveResult.Has(resultFromSystemAddr));
  EXPECT_FALSE(resolveResult.Has(resultFromCompetitorAddr));

  m_ctx->UnsubscribeAgent<AgentValidateCardinalityConstraint>();
  m_ctx->UnsubscribeAgent<AgentUpdateTrustReliability>();
  m_ctx->UnsubscribeAgent<AgentAggregateTrust>();
  m_ctx->UnsubscribeAgent<AgentResolveByTrustWeighted>();
}
