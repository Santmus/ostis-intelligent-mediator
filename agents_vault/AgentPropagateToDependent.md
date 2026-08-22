---
type: agent
tags: [agent, sc-agent, cooperation, state-machine]
action_class: action_propagate_to_dependent
version: (текущий коммит ostis_ps_user_ecosystem)
status: реализован, покрыт тестами
---

# AgentPropagateToDependent

← [[00 Индекс]] · из [[Кандидаты на реализацию]] (`rules_cooperation.scs`, вслед за [[AgentRegisterPartialResult]])

> [!note] Получатель — сама зависимая подзадача, не назначенная ей система
> В отличие от [[AgentDispatchReadySubtask]], где `rrel_receiver` — это
> система-исполнитель, здесь получателем inform-акта взята сама σ':
> на момент пропагации σ' может быть ещё не назначена (`task_allocator`
> ещё не отработал), а правило требует именно «сделать результат
> входом σ'» — адресат по смыслу текста правила сама подзадача.

## Идентификация

| Поле | Значение |
|---|---|
| Системный идентификатор (sc-IRI) класса действия | `action_propagate_to_dependent` |
| Тип sc-узла класса | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ класс | `AgentPropagateToDependent : public ScActionInitiatedAgent` |
| Файлы | `src/agent/agent_propagate_to_dependent.{hpp,cpp}` |
| Роль | Реализует `rule_propagate_to_dependent` (`agent_cooperative_executor`, `rules_cooperation.scs`) |
| Правило БЗ | `rules/rules_cooperation.scs` → `rule_propagate_to_dependent` |
| Модуль | `TrustModule` (см. [[AgentComputePiMetric]] — тот же аргумент про один модуль на `.dylib`) |

## Назначение

**Проблема.** Завершение подзадачи σ само по себе не делает её
результат видимым для подзадач σ', которые от неё зависят
(`nrel_subtask_depends_on`) — без явного шага их придётся опрашивать
или полагаться на внешний оркестратор.

**Цель.** Найти все σ', зависящие от только что завершённой σ, и
сделать результат r доступным каждой из них как вход — через
коммуникативный акт `concept_knowledge_exchange_act` типа
`concept_act_inform`.

**Ценность.** Замыкает граф зависимостей подзадач
(`nrel_subtask_depends_on`) в исполняемый push-механизм: как только σ
завершена, все ожидающие её σ' узнают об этом без опроса — тот же
принцип, что демонстрирует обмен `inform`-актами в
`scenario_knowledge_exchange.scs`.

**Стейкхолдеры:** `agent_cooperative_executor` (концептуальный «хозяин»
правила, фиксированный отправитель акта), подзадачи σ', ожидающие
входных данных от σ.

## Компетенция

**Capabilities:**
- проверить, что σ действительно `completed` (агент безопасен для
  спекулятивного вызова — как и [[AgentDispatchReadySubtask]]);
- найти все σ' с исходящей дугой `σ' -[nrel_subtask_depends_on]-> σ`
  (обратный обход — не тот же порядок, что читает
  `AllDependenciesCompleted` в [[AgentDispatchReadySubtask]]);
- для каждой найденной σ' сгенерировать `concept_knowledge_exchange_act`
  с ролями sender/receiver/act_type/payload;
- записать в узел-приёмник число пропагированных подзадач (0, если σ
  ещё не `completed` или зависимых σ' нет), создав sc-ссылку при первом
  обращении;
- корректно завершиться с ошибкой при отсутствии обязательных
  аргументов.

**Limitations (по коду):**
- **не заполняет** `rrel_context`/`rrel_timestamp` в создаваемых актах
  — тот же пробел, что и у [[AgentDispatchReadySubtask]]: правило не
  указывает источник контекста/времени.
- **отправитель акта — фиксированный `agent_cooperative_executor`**,
  не параметр — то же ограничение, что и у [[AgentDispatchReadySubtask]].
- **получатель — сама подзадача σ', а не назначенная ей система** — см.
  примечание выше; если в дальнейшем `task_allocator` назначит σ'
  систему, эта связь никак не используется агентом при построении акта.
- **не проверяет цикличность** графа зависимостей — как и
  [[AgentDispatchReadySubtask]], обнаружение цикла вне области этого
  агента (`rule_check_acyclicity`, не реализовано).
- **индикатор — не булев, а счётчик** пропагированных σ' — единственный
  агент проекта, где числовой индикатор не 0/1, а произвольное
  неотрицательное число.

## Вход

**Триггер:** дуга `action_initiated -> <экземпляр action_propagate_to_dependent>`.

| № | Роль | Тип | Обязательность | Валидация |
|---|---|---|---|---|
| 1 | `result` | частичный результат r, полученный по σ | обязателен | только `IsElement` |
| 2 | `subtask` | только что завершённая подзадача σ | обязателен | только `IsElement` |
| 3 | `result_level` | узел-приёмник индикатора | обязателен | только `IsElement` |

## Выход

Обновлённая/созданная sc-ссылка `value_link` на `result_level` со
значением — числом пропагированных зависимых подзадач (0, если σ ещё
не `completed`). Если пропагация произошла — дополнительно: по одному
новому `concept_knowledge_exchange_act` на каждую σ' (sender=
`agent_cooperative_executor`, receiver=σ', act_type=`concept_act_inform`,
payload=`result`). Успех — `FinishSuccessfully()` +
`FormResult(subtask, valueLink)`.

## Связи

**Зависимости:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`TargetOf`, `LinkByRole` — вынесен сюда вместе
с [[AgentRegisterPartialResult]], обобщение локального `LinkByRole` из
[[AgentDispatchReadySubtask]]), `keynodes/task_keynodes.hpp`
(`TaskKeynodes`).

**Читает:** `nrel_subtask_status` подзадачи σ, все σ' с исходящей
`nrel_subtask_depends_on` на σ (обратный обход).

**Пишет:** по одному `concept_knowledge_exchange_act` на каждую
найденную σ', `value_link` `result_level`.

**Workflow.** Третий, заключительный шаг жизненного цикла подзадачи в
`agent_cooperative_executor`: [[AgentDispatchReadySubtask]] →
[[AgentRegisterPartialResult]] (переводит σ в `completed`, что здесь и
читается как предусловие) → **AgentPropagateToDependent**.

## Поведение

```
IF  result не sc-элемент  OR  subtask не sc-элемент  OR  result_level не sc-элемент
THEN  FinishWithError()

ELSE
  status     := TargetOf(subtask, nrel_subtask_status)
  completed  := (status == concept_subtask_status_completed)
  count      := 0

  IF  completed
  THEN
    FOR EACH σ' WITH σ' -[nrel_subtask_depends_on]-> subtask:
      создать concept_knowledge_exchange_act:
        rrel_sender   = agent_cooperative_executor
        rrel_receiver = σ'
        rrel_act_type = concept_act_inform
        rrel_payload  = result
      count += 1

  записать count в value_link result_level (создать при отсутствии)
  action.FormResult(subtask, value_link)
  → action.FinishSuccessfully()
```

**Обработка ошибок/таймауты:** как у остальных агентов — `DoProgram`
синхронный, ожидание на стороне вызывающего кода через
`InitiateAndWait`.

## Безопасность

- Привилегии — как у остальных агентов `TrustModule`: без ACL, полный
  доступ к sc-памяти в рамках процесса `sc-server`.
- **Специфичный риск:** агент может разослать произвольный `result`
  ЛЮБОЙ подзадаче, найденной как зависимая от переданной `subtask`, без
  проверки, что вызывающая сторона имела на это право — то же общее
  для проекта отсутствие аутентификации инициатора действия, здесь с
  реальным побочным эффектом (коммуникативный акт на каждую σ').
- Не проверяется, что `result`/`subtask` действительно принадлежат
  соответствующим классам онтологии (только `IsElement`).
- Данные — не PII напрямую, но `concept_knowledge_exchange_act` несёт
  `rrel_payload=result`, который может содержать реальный
  пользовательский результат.

## Метрики

| Метрика | Значение / статус |
|---|---|
| Latency | Синхронно, без I/O; линейно по числу прямых зависимых подзадач (обычно единицы). |
| Accuracy | Не применимо — детерминированный обход графа, не предсказание; корректность проверена тестами на обоих исходах (completed/не completed). |
| Cost | Пренебрежимо мал. |
| Алерты | Не реализованы. |

## Тесты

Файл: `tests/test_agent_propagate_to_dependent.cpp` (3 теста, все проходят). Также покрыт вторым `TEST_F` в `tests/test_integration_request_pipeline.cpp` (`DispatchesRegistersAndPropagatesAlongSubtaskChain`), где выступает завершающим звеном цепочки Dispatch → Register → Propagate.

**1. `PropagatesToAllDependentsWhenSubtaskCompleted`**
Вход: σ `completed`, две зависимые σ' (`σ' -[depends_on]-> σ`).
Выход: индикатор `2.0`, обеим σ' создан inform-акт с
`rrel_receiver=σ'` и `rrel_payload=result`.

**2. `DoesNotPropagateWhenSubtaskNotCompleted`**
Вход: σ в статусе `in_progress`, одна зависимая σ'.
Выход: индикатор `0.0`, никакой акт не создан.

**3. `FinishesWithErrorOnMissingArguments`**
Вход: действие без аргументов.
Выход: `FinishedWithError`.
