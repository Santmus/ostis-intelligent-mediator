#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует механическое ядро, общее для шести SHACL-совместимых
// правил rules/rules_integrity_validation.scs (кроме
// rule_validate_provenance_chain -- это проверка транзитивного пути,
// не подсчёт дуг): подсчитать sc-дуги заданного отношения у целевого
// элемента и сравнить со счётом [minCount, maxCount].
//
// Каждое из шести правил — это один или несколько таких вызовов с
// разными (target, relation, mode, bounds, violation_class); какую
// именно комбинацию использовать для какого правила, задокументировано
// прямо в rules_integrity_validation.scs рядом с каждым правилом.
//
// Аргументы sc-действия action_validate_cardinality_constraint:
//   1) target          -- проверяемый sc-элемент
//   2) relation         -- существующее sc-отношение (nrel_*/rrel_*)
//   3) mode              -- один из трёх маркеров паттерна связи:
//                           concept_cardinality_check_outgoing_non_role,
//                           concept_cardinality_check_incoming_non_role,
//                           concept_cardinality_check_outgoing_role
//   4) min_count         -- числовая sc-ссылка, нижняя граница (включительно)
//   5) max_count         -- числовая sc-ссылка, верхняя граница (включительно);
//                           пустое/нечисловое содержимое трактуется как
//                           отсутствие верхней границы (maxCount не задан
//                           в исходном правиле -- так у 5 из 6 shape'ов)
//   6) violation_class   -- существующий sc_node_class нарушения
//                           (concept_cardinality_constraint_violation
//                           или concept_existence_constraint_violation
//                           -- правила используют разные классы для
//                           формально одинаковой механики)
//   7) result_level      -- узел для индикатора (1 -- в границах,
//                           0 -- нарушение); если у него ещё нет
//                           числовой sc-ссылки, она создаётся
//
// При нарушении (count вне [min_count, max_count]) создаётся экземпляр
// violation_class, подвешенный под result_level (тот же приём, что и
// у AgentDetectAnomalousRequestRate для concept_security_incident)
// и связанный с проверяемым target типизированным отношением
// concerns (concepts/integrity.scs, AGENTS_AUDIT.md п.6.8: прежде
// такой связи не было -- нарушение оставалось анонимным узлом).
// violatesConstraint (связь с самим ограничением, а не с целью
// проверки) и detectedIn (состояние жизненного цикла, в котором
// обнаружено нарушение) из того же пункта аудита не реализованы --
// это отдельные аргументы действия, которых сейчас нет ни у агента,
// ни у вызовов в rules_integrity_validation.scs.
class AgentValidateCardinalityConstraint : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
