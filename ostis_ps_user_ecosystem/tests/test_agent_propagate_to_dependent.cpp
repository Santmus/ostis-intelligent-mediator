#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_propagate_to_dependent.hpp"
#include "keynodes/task_keynodes.hpp"

using AgentPropagateToDependentTest = ScMemoryTest;

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

bool HasArc(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 = ctx.CreateIterator5(
      src, ScType::ConstCommonArc, trg, ScType::ConstPermPosArc, relationAddr);
  return it5->Next();
}
}  // namespace

// σ завершена и имеет двух зависимых σ' (σ' -[depends_on]-> σ), обе уже
// назначены исполнителям -- обеим устанавливается is_input_for(result,
// σ') и создаётся inform-акт с payload=result и receiver=исполнитель
// (не сама σ'), индикатор = 2.
TEST_F(AgentPropagateToDependentTest, PropagatesToAllDependentsWhenSubtaskCompleted)
{
  m_ctx->SubscribeAgent<AgentPropagateToDependent>();

  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_completed, TaskKeynodes::nrel_subtask_status);

  ScAddr const dependentOneAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const dependentTwoAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const executorOneAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const executorTwoAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, dependentOneAddr, subtaskAddr, TaskKeynodes::nrel_subtask_depends_on);
  LinkByRelation(*m_ctx, dependentTwoAddr, subtaskAddr, TaskKeynodes::nrel_subtask_depends_on);
  LinkByRelation(*m_ctx, dependentOneAddr, executorOneAddr, TaskKeynodes::nrel_assigned_to);
  LinkByRelation(*m_ctx, dependentTwoAddr, executorTwoAddr, TaskKeynodes::nrel_assigned_to);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_propagate_to_dependent);
  action.SetArguments(resultAddr, subtaskAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 2.0f, 0.0001f);

  for (auto const & [dependentAddr, executorAddr] :
       {std::pair{dependentOneAddr, executorOneAddr}, std::pair{dependentTwoAddr, executorTwoAddr}})
  {
    // result -[is_input_for]-> dependent -- подзадача не адресат сообщения,
    // а потребитель данных.
    EXPECT_TRUE(HasArc(*m_ctx, resultAddr, dependentAddr, TaskKeynodes::nrel_is_input_for));

    // Коммуникативный акт адресован назначенному исполнителю, не самой σ'.
    ScIterator5Ptr const actIt = m_ctx->CreateIterator5(
        ScType::ConstNode,
        ScType::ConstPermPosArc,
        executorAddr,
        ScType::ConstPermPosArc,
        TaskKeynodes::rrel_receiver);
    ASSERT_TRUE(actIt->Next());

    ScAddr const actAddr = actIt->Get(0);
    EXPECT_TRUE(m_ctx->CreateIterator3(actAddr, ScType::ConstPermPosArc, resultAddr)->Next());
    EXPECT_TRUE(
        m_ctx->CreateIterator3(actAddr, ScType::ConstPermPosArc, TaskKeynodes::concept_act_inform)->Next());

    // Сама подзадача НЕ является получателем ни одного акта.
    EXPECT_FALSE(m_ctx->CreateIterator5(
                         ScType::ConstNode,
                         ScType::ConstPermPosArc,
                         dependentAddr,
                         ScType::ConstPermPosArc,
                         TaskKeynodes::rrel_receiver)
                     ->Next());
  }

  m_ctx->UnsubscribeAgent<AgentPropagateToDependent>();
}

// σ завершена, но зависимая σ' ещё не назначена исполнителю -- результат
// всё равно становится входом σ' (is_input_for), индикатор учитывает σ',
// но коммуникативный акт не создаётся -- адресовать его некому.
TEST_F(AgentPropagateToDependentTest, MarksInputForUnassignedDependentWithoutCommunicationAct)
{
  m_ctx->SubscribeAgent<AgentPropagateToDependent>();

  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_completed, TaskKeynodes::nrel_subtask_status);

  ScAddr const dependentAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, dependentAddr, subtaskAddr, TaskKeynodes::nrel_subtask_depends_on);
  // Нарочно не назначаем исполнителя dependentAddr.

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_propagate_to_dependent);
  action.SetArguments(resultAddr, subtaskAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_TRUE(HasArc(*m_ctx, resultAddr, dependentAddr, TaskKeynodes::nrel_is_input_for));

  EXPECT_FALSE(m_ctx->CreateIterator5(
                       ScType::ConstNode,
                       ScType::ConstPermPosArc,
                       dependentAddr,
                       ScType::ConstPermPosArc,
                       TaskKeynodes::rrel_receiver)
                   ->Next());

  m_ctx->UnsubscribeAgent<AgentPropagateToDependent>();
}

// σ ещё не completed -- ничего не пропагируется, индикатор 0, это не
// ошибка (агент безопасен для спекулятивного вызова).
TEST_F(AgentPropagateToDependentTest, DoesNotPropagateWhenSubtaskNotCompleted)
{
  m_ctx->SubscribeAgent<AgentPropagateToDependent>();

  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_in_progress, TaskKeynodes::nrel_subtask_status);

  ScAddr const dependentAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, dependentAddr, subtaskAddr, TaskKeynodes::nrel_subtask_depends_on);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_propagate_to_dependent);
  action.SetArguments(resultAddr, subtaskAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  EXPECT_FALSE(m_ctx->CreateIterator5(
                       ScType::ConstNode,
                       ScType::ConstPermPosArc,
                       dependentAddr,
                       ScType::ConstPermPosArc,
                       TaskKeynodes::rrel_receiver)
                   ->Next());

  m_ctx->UnsubscribeAgent<AgentPropagateToDependent>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentPropagateToDependentTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentPropagateToDependent>();

  ScAction action = m_ctx->GenerateAction(TaskKeynodes::action_propagate_to_dependent);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentPropagateToDependent>();
}
