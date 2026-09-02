#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_record_prov_entity.hpp"
#include "keynodes/provenance_keynodes.hpp"

using AgentRecordProvEntityTest = ScMemoryTest;

namespace
{
ScAddr TargetOf(ScAgentContext & ctx, ScAddr const & src, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(src, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}
}  // namespace

// Базовый случай: результат, деятельность и агент существуют --
// создаётся PROV Entity, связанная wasGeneratedBy/wasAttributedTo/
// has_provenance.
TEST_F(AgentRecordProvEntityTest, RecordsEntityWithGeneratedByAndAttributedTo)
{
  m_ctx->SubscribeAgent<AgentRecordProvEntity>();

  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const activityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const agentAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_record_prov_entity);
  action.SetArguments(resultAddr, activityAddr, agentAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const entityAddr = TargetOf(*m_ctx, resultAddr, ProvenanceKeynodes::nrel_has_provenance);
  ASSERT_TRUE(m_ctx->IsElement(entityAddr));
  EXPECT_TRUE(m_ctx->CheckConnector(ProvenanceKeynodes::concept_prov_entity, entityAddr, ScType::ConstPermPosArc));
  EXPECT_EQ(TargetOf(*m_ctx, entityAddr, ProvenanceKeynodes::nrel_prov_was_generated_by), activityAddr);
  EXPECT_EQ(TargetOf(*m_ctx, entityAddr, ProvenanceKeynodes::nrel_prov_was_attributed_to), agentAddr);

  m_ctx->UnsubscribeAgent<AgentRecordProvEntity>();
}

// has_provenance функционально: повторный вызов на том же result
// заменяет старую цель новой сущностью, а не добавляет вторую дугу.
TEST_F(AgentRecordProvEntityTest, ReplacesExistingProvenanceTargetFunctionally)
{
  m_ctx->SubscribeAgent<AgentRecordProvEntity>();

  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const activityAddr1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const agentAddr1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const activityAddr2 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const agentAddr2 = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction firstAction = m_ctx->GenerateAction(ProvenanceKeynodes::action_record_prov_entity);
  firstAction.SetArguments(resultAddr, activityAddr1, agentAddr1);
  EXPECT_TRUE(firstAction.InitiateAndWait(2000));
  EXPECT_TRUE(firstAction.IsFinishedSuccessfully());

  ScAction secondAction = m_ctx->GenerateAction(ProvenanceKeynodes::action_record_prov_entity);
  secondAction.SetArguments(resultAddr, activityAddr2, agentAddr2);
  EXPECT_TRUE(secondAction.InitiateAndWait(2000));
  EXPECT_TRUE(secondAction.IsFinishedSuccessfully());

  ScIterator5Ptr const it5 = m_ctx->CreateIterator5(
      resultAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc,
      ProvenanceKeynodes::nrel_has_provenance);
  ASSERT_TRUE(it5->Next());
  ScAddr const secondEntityAddr = it5->Get(2);
  EXPECT_FALSE(it5->Next());  // ровно одна цель
  EXPECT_EQ(TargetOf(*m_ctx, secondEntityAddr, ProvenanceKeynodes::nrel_prov_was_generated_by), activityAddr2);

  m_ctx->UnsubscribeAgent<AgentRecordProvEntity>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentRecordProvEntityTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentRecordProvEntity>();

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_record_prov_entity);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentRecordProvEntity>();
}
