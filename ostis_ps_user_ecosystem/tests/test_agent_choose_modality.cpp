#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_choose_modality.hpp"
#include "keynodes/adaptive_interface_keynodes.hpp"

using AgentChooseModalityTest = ScMemoryTest;

namespace
{
void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, trg);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

// Ищет среди детей resultLevelAddr узел класса modalityAddr (сам
// modalityAddr, поскольку модальность -- уже существующий разделяемый
// класс, а не свежий экземпляр).
bool HasModalityChild(ScAgentContext & ctx, ScAddr const & resultLevelAddr, ScAddr const & modalityAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, modalityAddr);
  return it3->Next();
}
}  // namespace

// situation классифицирован concept_visual_perception_constraint --
// выбирается concept_modality_voice независимо от предпочтений.
TEST_F(AgentChooseModalityTest, ChoosesVoiceWhenVisualPerceptionConstrained)
{
  m_ctx->SubscribeAgent<AgentChooseModality>();

  ScAddr const situationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, AdaptiveInterfaceKeynodes::concept_visual_perception_constraint, situationAddr);

  ScAddr const userModelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const preferenceAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, AdaptiveInterfaceKeynodes::concept_preference_visual_modality, preferenceAddr);
  LinkByRelation(*m_ctx, userModelAddr, preferenceAddr, AdaptiveInterfaceKeynodes::nrel_has_preference);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_choose_modality);
  action.SetArguments(situationAddr, userModelAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_TRUE(HasModalityChild(*m_ctx, resultLevelAddr, AdaptiveInterfaceKeynodes::concept_modality_voice));
  EXPECT_FALSE(HasModalityChild(*m_ctx, resultLevelAddr, AdaptiveInterfaceKeynodes::concept_modality_visual));

  m_ctx->UnsubscribeAgent<AgentChooseModality>();
}

// situation без ограничения, но пользователь имеет предпочтение
// visual_modality -- выбирается concept_modality_visual.
TEST_F(AgentChooseModalityTest, ChoosesVisualWhenPreferenceVisualPresent)
{
  m_ctx->SubscribeAgent<AgentChooseModality>();

  ScAddr const situationAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без ограничения

  ScAddr const userModelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const otherPreferenceAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без visual_modality
  ScAddr const visualPreferenceAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, AdaptiveInterfaceKeynodes::concept_preference_visual_modality, visualPreferenceAddr);
  LinkByRelation(*m_ctx, userModelAddr, otherPreferenceAddr, AdaptiveInterfaceKeynodes::nrel_has_preference);
  LinkByRelation(*m_ctx, userModelAddr, visualPreferenceAddr, AdaptiveInterfaceKeynodes::nrel_has_preference);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_choose_modality);
  action.SetArguments(situationAddr, userModelAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_TRUE(HasModalityChild(*m_ctx, resultLevelAddr, AdaptiveInterfaceKeynodes::concept_modality_visual));

  m_ctx->UnsubscribeAgent<AgentChooseModality>();
}

// Ни ограничения, ни предпочтения нет -- выбирается concept_modality_text
// по умолчанию.
TEST_F(AgentChooseModalityTest, DefaultsToTextWhenNoConditionApplies)
{
  m_ctx->SubscribeAgent<AgentChooseModality>();

  ScAddr const situationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const userModelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const preferenceAddr = m_ctx->GenerateNode(ScType::ConstNode);  // не visual_modality
  LinkByRelation(*m_ctx, userModelAddr, preferenceAddr, AdaptiveInterfaceKeynodes::nrel_has_preference);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_choose_modality);
  action.SetArguments(situationAddr, userModelAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_TRUE(HasModalityChild(*m_ctx, resultLevelAddr, AdaptiveInterfaceKeynodes::concept_modality_text));

  m_ctx->UnsubscribeAgent<AgentChooseModality>();
}

// Оба условия выполнены одновременно -- ограничение восприятия имеет
// приоритет над предпочтением (порядок ветвления If/Elif правила).
TEST_F(AgentChooseModalityTest, VisualConstraintTakesPriorityOverPreference)
{
  m_ctx->SubscribeAgent<AgentChooseModality>();

  ScAddr const situationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, AdaptiveInterfaceKeynodes::concept_visual_perception_constraint, situationAddr);

  ScAddr const userModelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const visualPreferenceAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, AdaptiveInterfaceKeynodes::concept_preference_visual_modality, visualPreferenceAddr);
  LinkByRelation(*m_ctx, userModelAddr, visualPreferenceAddr, AdaptiveInterfaceKeynodes::nrel_has_preference);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_choose_modality);
  action.SetArguments(situationAddr, userModelAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_TRUE(HasModalityChild(*m_ctx, resultLevelAddr, AdaptiveInterfaceKeynodes::concept_modality_voice));
  EXPECT_FALSE(HasModalityChild(*m_ctx, resultLevelAddr, AdaptiveInterfaceKeynodes::concept_modality_visual));

  m_ctx->UnsubscribeAgent<AgentChooseModality>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentChooseModalityTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentChooseModality>();

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_choose_modality);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentChooseModality>();
}
