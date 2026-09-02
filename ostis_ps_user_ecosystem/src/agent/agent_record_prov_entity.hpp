#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_auto_record_prov_entity (rules/rules_provenance_completion.scs,
// agent_provenance_registrar): для результата обработки r создаёт
// PROV-O сущность e_r (concept_prov_entity), связывает её с породившей
// activity через wasGeneratedBy, с породившим agent'ом через
// wasAttributedTo, и связывает r с e_r через has_provenance.
//
// Аргументы sc-действия action_record_prov_entity:
//   1) result   -- r, concept_processing_result (агент доверяет
//                  вызывающей стороне относительно класса, как и везде
//                  в модуле)
//   2) activity -- prov_activity_of_producer, concept_prov_activity,
//                  породившая r (обычно создаётся заранее
//                  AgentRecordProvActivity, но эта связь не проверяется
//                  -- каждый агент независим)
//   3) agent    -- prov_agent_of_producer, concept_prov_agent, автор r
//
// "When: new concept_processing_result(r) появляется в sc-памяти" не
// реализовано как автоматический триггер -- ни один агент проекта не
// подписан на событие появления произвольного узла; вызывающая сторона
// инициирует регистрацию явно, как и везде в модуле (см. также
// комментарий в rules_provenance_completion.scs).
//
// has_provenance функционально (relations/relations_provenance.scs) --
// используется SetFunctionalRelationTarget, а не LinkByRelation, чтобы
// корректно заменить существующую цель, если она уже есть.
//
// Наблюдение (не исправляется здесь): AgentRegisterPartialResult
// (rules/rules_cooperation.scs) уже создаёт для каждого частичного
// результата свою провенансную запись -- обобщённый узел, классифицированный
// TaskKeynodes::concept_provenance_record (без PROV-O специализации на
// Entity/Activity/Agent) и связанный через тот же nrel_has_provenance.
// Оба механизма используют одно и то же (функциональное) отношение;
// вызов AgentRecordProvEntity ПОСЛЕ AgentRegisterPartialResult заменит
// более общую запись специализированной PROV-сущностью (has_provenance
// функционально -- старая цель стирается SetFunctionalRelationTarget).
// Единого решения, какой из двух механизмов "канонический", текст
// модели не даёт -- это наблюдение, а не исправляемое здесь противоречие.
//
// Не реализовано:
//  - PROV-активность/агент, которые связываются с r, не создаются этим
//    агентом -- ожидаются уже существующими (например, через
//    AgentRecordProvActivity), передаются готовыми аргументами;
//  - только IsElement на аргументах, как и у остальных агентов модуля.
class AgentRecordProvEntity : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
