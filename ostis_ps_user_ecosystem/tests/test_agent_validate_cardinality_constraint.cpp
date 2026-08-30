#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_validate_cardinality_constraint.hpp"
#include "keynodes/integrity_keynodes.hpp"

using AgentValidateCardinalityConstraintTest = ScMemoryTest;

namespace
{
ScAddr MakeFloatLink(ScAgentContext & ctx, float value)
{
  ScAddr const linkAddr = ctx.GenerateLink(ScType::ConstNodeLink);
  ctx.SetLinkContent(linkAddr, std::to_string(value));
  return linkAddr;
}

// Ссылка без содержимого -- в агенте трактуется как "верхняя граница не
// задана" (5 из 6 SHACL-shape'ов проекта указывают только minCount).
ScAddr MakeEmptyLink(ScAgentContext & ctx)
{
  return ctx.GenerateLink(ScType::ConstNodeLink);
}

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

// Ищет среди детей resultLevelAddr узел заданного класса нарушения, не
// полагаясь на различение sc-ссылки/узла типом в запросе итератора
// (тот же приём, что и в test_agent_detect_anomalous_request_rate.cpp).
// Возвращает ScAddr::Empty, если такого нарушения нет.
ScAddr FindViolationChild(ScAgentContext & ctx, ScAddr const & resultLevelAddr, ScAddr const & violationClassAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (it3->Next())
  {
    ScAddr const childAddr = it3->Get(2);
    if (ctx.CheckConnector(violationClassAddr, childAddr, ScType::ConstPermPosArc))
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

// Воспроизводит shape_result_provenance (nrel_has_provenance, minCount 1,
// без maxCount): у результата три исходящих связи -- заведомо в границах.
TEST_F(AgentValidateCardinalityConstraintTest, ValidatesWhenCountWithinBounds)
{
  m_ctx->SubscribeAgent<AgentValidateCardinalityConstraint>();

  ScAddr const relationAddr = m_ctx->GenerateNode(ScType::ConstNodeNonRole);
  ScAddr const targetAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, targetAddr, m_ctx->GenerateNode(ScType::ConstNode), relationAddr);
  LinkByRelation(*m_ctx, targetAddr, m_ctx->GenerateNode(ScType::ConstNode), relationAddr);
  LinkByRelation(*m_ctx, targetAddr, m_ctx->GenerateNode(ScType::ConstNode), relationAddr);

  ScAddr const minCountAddr = MakeFloatLink(*m_ctx, 1.0f);
  ScAddr const maxCountAddr = MakeEmptyLink(*m_ctx);
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(IntegrityKeynodes::action_validate_cardinality_constraint);
  action.SetArguments(
      targetAddr,
      relationAddr,
      IntegrityKeynodes::concept_cardinality_check_outgoing_non_role,
      minCountAddr,
      maxCountAddr,
      IntegrityKeynodes::concept_existence_constraint_violation,
      resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_FALSE(m_ctx->IsElement(
      FindViolationChild(*m_ctx, resultLevelAddr, IntegrityKeynodes::concept_existence_constraint_violation)));

  m_ctx->UnsubscribeAgent<AgentValidateCardinalityConstraint>();
}

// Воспроизводит shape_user_request_authorship (nrel_formulates⁻¹,
// minCount 1, maxCount 1): у запроса нет ни одного входящего
// nrel_formulates -- нарушение кардинального ограничения.
TEST_F(AgentValidateCardinalityConstraintTest, CreatesViolationWhenCountBelowMinimum)
{
  m_ctx->SubscribeAgent<AgentValidateCardinalityConstraint>();

  ScAddr const relationAddr = m_ctx->GenerateNode(ScType::ConstNodeNonRole);
  ScAddr const targetAddr = m_ctx->GenerateNode(ScType::ConstNode);
  // Нарочно не связываем ни одного пользователя через relationAddr.

  ScAddr const minCountAddr = MakeFloatLink(*m_ctx, 1.0f);
  ScAddr const maxCountAddr = MakeFloatLink(*m_ctx, 1.0f);
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(IntegrityKeynodes::action_validate_cardinality_constraint);
  action.SetArguments(
      targetAddr,
      relationAddr,
      IntegrityKeynodes::concept_cardinality_check_incoming_non_role,
      minCountAddr,
      maxCountAddr,
      IntegrityKeynodes::concept_cardinality_constraint_violation,
      resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);
  ScAddr const violationAddr =
      FindViolationChild(*m_ctx, resultLevelAddr, IntegrityKeynodes::concept_cardinality_constraint_violation);
  ASSERT_TRUE(m_ctx->IsElement(violationAddr));

  // Нарушение типизированно связано с проверяемой сущностью (concerns).
  EXPECT_EQ(TargetOf(*m_ctx, violationAddr, IntegrityKeynodes::nrel_concerns), targetAddr);

  m_ctx->UnsubscribeAgent<AgentValidateCardinalityConstraint>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentValidateCardinalityConstraintTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentValidateCardinalityConstraint>();

  ScAction action = m_ctx->GenerateAction(IntegrityKeynodes::action_validate_cardinality_constraint);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentValidateCardinalityConstraint>();
}
