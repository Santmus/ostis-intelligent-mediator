---
type: agent
tags: [agent, sc-agent, cooperation, provenance]
action_class: action_register_partial_result
version: (текущий коммит ostis_ps_user_ecosystem)
status: реализован, покрыт тестами
---

# AgentRegisterPartialResult

← [[00 Индекс]] · из [[Кандидаты на реализацию]] (`rules_cooperation.scs`, вслед за [[AgentDispatchReadySubtask]])

> [!note] Без индикатора готовности
> В отличие от [[AgentDispatchReadySubtask]] и [[AgentPropagateToDependent]],
> здесь нет условия готовности — правило безусловно фиксирует факт
> получения результата. Поэтому агент не спекулятивен и не пишет
> числовой индикатор в узел-приёмник: единственный путь к неуспеху —
> отсутствующий аргумент (`FinishWithError`).

## Идентификация

| Поле | Значение |
|---|---|
| Системный идентификатор (sc-IRI) класса действия | `action_register_partial_result` |
| Тип sc-узла класса | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ класс | `AgentRegisterPartialResult : public ScActionInitiatedAgent` |
| Файлы | `src/agent/agent_register_partial_result.{hpp,cpp}` |
| Роль | Реализует `rule_register_partial_result` (`agent_cooperative_executor`, `rules_cooperation.scs`) |
| Правило БЗ | `rules/rules_cooperation.scs` → `rule_register_partial_result` |
| Модуль | `TrustModule` (см. [[AgentComputePiMetric]] — тот же аргумент про один модуль на `.dylib`) |

## Назначение

**Проблема.** Система-исполнитель присылает частичный результат по
подзадаче, но без явной фиксации этого факта в sc-памяти результат
остаётся «неучтённым»: не классифицирован, не связан ни с
произведшей его системой, ни с породившей его подзадачей, ни с
контекстом взаимодействия — а сама подзадача формально всё ещё
считается незавершённой.

**Цель.** Атомарно зафиксировать провенансный триплет
`produces`/`caused_by_subtask`/`generated_in_context`, классифицировать
результат как `concept_partial_result` и перевести подзадачу в
`completed` — единым, детерминированным шагом.

**Ценность.** Реализует ту же логику, что описана в
`axioms/axioms_provenance.scs` (шаблон автоматической инстанциации при
исполнении подзадачи) и воспроизведена как фикстура в
`scenario_request_processing.scs` (шаг 5, `result_routes`).

**Стейкхолдеры:** `agent_cooperative_executor` (концептуальный «хозяин»
правила), [[AgentPropagateToDependent]] (следующий шаг того же
жизненного цикла подзадачи — читает `completed`-статус, который здесь
устанавливается), `agent_provenance_registrar` (дополняет тот же
результат PROV-O связями `wasGeneratedBy`/`wasAttributedTo` — вне
области этого агента).

## Компетенция

**Capabilities:**
- классифицировать переданный узел результата как `concept_partial_result`;
- создать три нероле-вые бинарные связи: `produces(s, r)`,
  `caused_by_subtask(r, σ)`, `generated_in_context(r, C)`;
- перевести `nrel_subtask_status(σ)` в `completed` (с учётом того, что
  отношение функционально — старая дуга стирается, а не дублируется,
  `SetFunctionalRelationTarget`);
- корректно завершиться с ошибкой при отсутствии любого из четырёх
  обязательных аргументов.

**Limitations (по коду):**
- **не заполняет** `wasGeneratedBy`/`wasAttributedTo` (PROV-O часть
  того же шаблона, `axioms_provenance.scs`) — по тексту правила это
  зона ответственности `agent_provenance_registrar`, а не
  `agent_cooperative_executor`.
- **безусловен**: не проверяет, что подзадача действительно была в
  статусе `in_progress` перед завершением, — правило не формулирует
  такое предусловие, а сам агент лишь фиксирует факт «результат
  получен».
- **не проверяет типы аргументов** (только `IsElement`) — произвольные
  узлы могут быть связаны как «система»/«результат»/«подзадача»/
  «контекст» без проверки принадлежности соответствующим классам
  онтологии.
- **не дедуплицирует** провенансные связи — повторный вызов с теми же
  аргументами создаст вторые экземпляры `produces`/`caused_by_subtask`/
  `generated_in_context` (не функциональные отношения, в отличие от
  статуса), потенциальный источник дублей при неидемпотентном внешнем
  вызове.

## Вход

**Триггер:** дуга `action_initiated -> <экземпляр action_register_partial_result>`.

| № | Роль | Тип | Обязательность | Валидация |
|---|---|---|---|---|
| 1 | `system` | `concept_intelligent_system` s, приславшая результат | обязателен | только `IsElement` |
| 2 | `result` | результат r (ещё не классифицирован) | обязателен | только `IsElement` |
| 3 | `subtask` | подзадача σ, по которой получен результат | обязателен | только `IsElement` |
| 4 | `context` | контекст взаимодействия C | обязателен | только `IsElement` |

## Выход

Классификация `result <- concept_partial_result`; три новые связи
`produces(system, result)`, `caused_by_subtask(result, subtask)`,
`generated_in_context(result, context)`; `nrel_subtask_status(subtask)`
переведён в `concept_subtask_status_completed`. Успех —
`FinishSuccessfully()` + `FormResult(result, subtask)`.

## Связи

**Зависимости:** `common/relation_io.hpp` (`LinkByRelation`,
`SetFunctionalRelationTarget` — вынесен сюда при реализации этого
агента вместе с [[AgentPropagateToDependent]], обобщение локального
`SetFunctionalStatus` из [[AgentDispatchReadySubtask]]),
`keynodes/task_keynodes.hpp` (`TaskKeynodes`).

**Читает:** ничего (аргументы принимаются как готовые sc-элементы,
предусловий на их текущее состояние нет).

**Пишет:** классификацию `result`, три `nrel_*`-связи, функциональный
`nrel_subtask_status` подзадачи.

**Workflow.** Второй шаг жизненного цикла подзадачи в
`agent_cooperative_executor`: [[AgentDispatchReadySubtask]]
(`proposed → in_progress`) → **AgentRegisterPartialResult**
(`in_progress → completed` + провенанс) → [[AgentPropagateToDependent]]
(рассылка результата зависимым подзадачам). Связаны общим состоянием
`nrel_subtask_status` конкретной подзадачи, а не оркестратором.

## Поведение

```
IF  любой из {system, result, subtask, context} не sc-элемент
THEN  FinishWithError()

ELSE
  result <- concept_partial_result
  LinkByRelation(system, result, nrel_produces)
  LinkByRelation(result, subtask, nrel_caused_by_subtask)
  LinkByRelation(result, context, nrel_generated_in_context)

  SetFunctionalRelationTarget(subtask, nrel_subtask_status,
                               concept_subtask_status_completed)
    // функциональное отношение: старая дуга стирается перед созданием новой

  action.FormResult(result, subtask)
  → action.FinishSuccessfully()
```

**Обработка ошибок/таймауты:** как у остальных агентов — `DoProgram`
синхронный, ожидание на стороне вызывающего кода через
`InitiateAndWait`.

## Безопасность

- Привилегии — как у остальных агентов `TrustModule`: без ACL, полный
  доступ к sc-памяти в рамках процесса `sc-server`.
- **Специфичный риск:** агент безусловно переводит ЛЮБУЮ переданную
  подзадачу в `completed` и классифицирует ЛЮБОЙ узел как результат,
  без проверки, что вызывающая сторона действительно является
  назначенной системой — то же общее для проекта отсутствие
  аутентификации инициатора действия.
- Данные (идентификаторы систем, содержимое результата) — не PII
  напрямую, но `result` может нести реальные пользовательские данные,
  а связь `generated_in_context` — привязку к контексту взаимодействия.

## Метрики

| Метрика | Значение / статус |
|---|---|
| Latency | Синхронно, без I/O; константное число операций (не зависит от размера БЗ). |
| Accuracy | Не применимо — детерминированная фиксация фактов, не предсказание; корректность проверена тестами. |
| Cost | Пренебрежимо мал. |
| Алерты | Не реализованы. |

## Тесты

Файл: `tests/test_agent_register_partial_result.cpp` (3 теста, все проходят). Также покрыт вторым `TEST_F` в `tests/test_integration_request_pipeline.cpp` (`DispatchesRegistersAndPropagatesAlongSubtaskChain`), где выступает средним звеном цепочки Dispatch → Register → Propagate.

**1. `RegistersPartialResultAndCompletesSubtask`**
Вход: подзадача в статусе `in_progress`, все 4 аргумента — существующие узлы.
Выход: `result` классифицирован как `concept_partial_result`, все три
провенансные связи созданы с ожидаемыми концами, статус подзадачи —
`completed` (и единственный).

**2. `SetsCompletedEvenWithoutPriorStatus`**
Вход: подзадача вообще без предшествующей дуги `nrel_subtask_status`.
Выход: статус всё равно устанавливается в `completed` — подтверждает
безусловность агента.

**3. `FinishesWithErrorOnMissingArguments`**
Вход: действие без аргументов.
Выход: `FinishedWithError`.
