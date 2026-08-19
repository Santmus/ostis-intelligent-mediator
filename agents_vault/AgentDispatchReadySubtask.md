---
type: agent
tags: [agent, sc-agent, cooperation, state-machine]
action_class: action_dispatch_ready_subtask
version: (текущий коммит ostis_ps_user_ecosystem)
status: реализован, покрыт тестами, проверен на живом sc-server
---

# AgentDispatchReadySubtask

← [[00 Индекс]] · из [[Кандидаты на реализацию]] (топ-5, #4)

> [!note] Один аргумент, а не пять
> В отличие от остальных агентов проекта, здесь не нужно передавать
> исполнителя, статусы зависимостей и т.д. отдельными аргументами — всё
> это уже связано с самой подзадачей через существующие отношения
> онтологии (`nrel_assigned_to`, `nrel_subtask_status`,
> `nrel_subtask_depends_on`) и вычитывается по одному переданному узлу.

## Идентификация

| Поле | Значение |
|---|---|
| Системный идентификатор (sc-IRI) класса действия | `action_dispatch_ready_subtask` |
| Тип sc-узла класса | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ класс | `AgentDispatchReadySubtask : public ScActionInitiatedAgent` |
| Файлы | `src/agent/agent_dispatch_ready_subtask.{hpp,cpp}` |
| Роль | Реализует `rule_dispatch_ready_subtask` (`agent_cooperative_executor`, `rules_cooperation.scs`) |
| Правило БЗ | `rules/rules_cooperation.scs` → `rule_dispatch_ready_subtask` |
| Модуль | `TrustModule` (см. [[AgentComputePiMetric]] — тот же аргумент про один модуль на `.dylib`) |

## Назначение

**Проблема.** Подзадача может быть назначена исполнителю, но не готова
к запуску, пока не завершены подзадачи, от которых она зависит
(`nrel_subtask_depends_on`) — отправить её раньше времени означало бы
запустить обработку без нужных входных данных.

**Цель.** Определить момент, когда подзадача действительно готова
(назначена + статус «предложена» + все зависимости завершены), перевести
её в состояние «в работе» и уведомить исполнителя коммуникативным актом.

**Ценность.** Формализует переход состояния подзадачи как проверяемое
условие, а не эвристику «наверное, пора» — тот же принцип
детерминированного state-машина перехода, что описан в
`scenario_request_processing.scs`.

**Стейкхолдеры:** `agent_cooperative_executor` (концептуальный «хозяин»
правила, используется как формальный отправитель акта), интеллектуальные
системы-исполнители, ожидающие подзадачи.

## Компетенция

**Capabilities:**
- проверить триаду условий готовности: назначен исполнитель, статус
  `proposed`, все зависимости `completed` (пустой список зависимостей
  считается выполненным вакуумно);
- при выполнении — сменить статус на `in_progress` (с учётом того, что
  `nrel_subtask_status` функционально: старая дуга стирается, а не
  дублируется) и сгенерировать `concept_knowledge_exchange_act` с ролями
  sender/receiver/act_type/payload;
- записать булев индикатор результата (1.0/0.0) в узел-приёмник,
  создав sc-ссылку при первом обращении;
- корректно завершиться с ошибкой при отсутствии обязательных
  аргументов.

**Limitations (по коду):**
- **не заполняет** `rrel_context`/`rrel_timestamp` в создаваемом
  `concept_knowledge_exchange_act`, хотя определение акта в
  `concepts/knowledge_exchange.scs` их предполагает — правило не
  указывает источник контекста/времени, а агенту неоткуда взять
  текущее время или актуальный контекст взаимодействия.
- **отправитель акта — фиксированный `agent_cooperative_executor`**,
  не параметр: единственный реализованный вариант «хозяина» правила в
  каталоге `concepts/sc_agents.scs`; выбран как уже существующий
  формальный элемент, а не выдуман, но это делает отправителя
  константой, а не настоящим динамическим полем.
- **«не готова к запуску» — не ошибка, а индикатор 0**: агент рассчитан
  на спекулятивный вызов (например, оркестратором на каждой
  `proposed`-подзадаче периодически) — намеренное отличие от
  [[AgentResolveByTrustWeighted]], где пустой вход — ошибка.
- **не проверяет цикличность** графа зависимостей — если
  `nrel_subtask_depends_on` образует цикл, агент просто никогда не
  сочтёт такую подзадачу готовой; обнаружение цикла — задача
  `rule_check_acyclicity` (`rules_decomposition.scs`, не реализовано).
- статус читается напрямую как `TargetOf(...) == concept_subtask_status_proposed`
  без проверки, что у подзадачи вообще есть хоть какой-то статус —
  отсутствие статуса и «неправильный» статус неразличимы (оба дают
  `ready = false`).

## Вход

**Триггер:** дуга `action_initiated -> <экземпляр action_dispatch_ready_subtask>`.

| № | Роль | Тип | Обязательность | Валидация |
|---|---|---|---|---|
| 1 | `subtask` | существующий экземпляр `concept_subtask` | обязателен | только `IsElement` |
| 2 | `result_level` | узел-приёмник индикатора | обязателен | только `IsElement` |

## Выход

Обновлённая/созданная sc-ссылка `value_link` на `result_level` с
`1.0` (диспетчеризовано) или `0.0` (условие не выполнено). Если
диспетчеризовано — дополнительно: смена `nrel_subtask_status` подзадачи
на `concept_subtask_status_in_progress`, новый
`concept_knowledge_exchange_act` (sender=`agent_cooperative_executor`,
receiver=назначенная система, act_type=`concept_act_request`,
payload=подзадача). Успех — `FinishSuccessfully()` +
`FormResult(subtask, valueLink)`.

## Связи

**Зависимости:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`TargetOf`/`LinkByRelation` — вынесены сюда
именно при реализации этого агента, так как понадобились во второй раз
после [[AgentResolveByTrustWeighted]]), `keynodes/task_keynodes.hpp`
(`TaskKeynodes`).

**Читает:** `nrel_assigned_to`, `nrel_subtask_status` подзадачи и всех
её прямых зависимостей (`nrel_subtask_depends_on`).

**Пишет:** `nrel_subtask_status` подзадачи (при готовности),
`concept_knowledge_exchange_act` со связанными ролями, `value_link`
`result_level`.

**Workflow.** Независим от остальных агентов проекта по данным —
единственный, оперирующий доменом «подзадачи» (`concept_subtask`), а не
доверием/провенансом/интеграцией результатов.

## Поведение

```
IF  subtask не sc-элемент  OR  result_level не sc-элемент
THEN  FinishWithError()

ELSE
  s      := TargetOf(subtask, nrel_assigned_to)
  status := TargetOf(subtask, nrel_subtask_status)

  ready := IsElement(s)
           AND status == concept_subtask_status_proposed
           AND ∀ dep IN subtask -[nrel_subtask_depends_on]->:
                 TargetOf(dep, nrel_subtask_status) == concept_subtask_status_completed
           // пустой список зависимостей -> "для всех" истинно вакуумно

  IF  ready
  THEN
    сменить nrel_subtask_status(subtask) на concept_subtask_status_in_progress
      // функциональное отношение: старая дуга стирается перед созданием новой
    создать concept_knowledge_exchange_act:
      rrel_sender   = agent_cooperative_executor
      rrel_receiver = s
      rrel_act_type = concept_act_request
      rrel_payload  = subtask

  записать (ready ? 1.0 : 0.0) в value_link result_level (создать при отсутствии)
  action.FormResult(subtask, value_link)
  → action.FinishSuccessfully()
```

**Обработка ошибок/таймауты:** как у остальных агентов — `DoProgram`
синхронный, ожидание на стороне вызывающего кода через
`InitiateAndWait`.

## Безопасность

- Привилегии — как у остальных агентов `TrustModule`: без ACL, полный
  доступ к sc-памяти в рамках процесса `sc-server`.
- **Специфичный риск:** агент может сменить статус ЛЮБОЙ переданной
  подзадачи на `in_progress` и разослать запрос ЛЮБОЙ системе,
  указанной в `nrel_assigned_to`, без проверки, что вызывающая сторона
  имела на это право — то же общее для проекта отсутствие
  аутентификации инициатора действия, но здесь с реальным побочным
  эффектом (коммуникативный акт, а не просто число).
- Не проверяется, что `subtask` действительно `concept_subtask` (только
  `IsElement`) — произвольный узел может быть переведён в
  «состояние in_progress» и породить фиктивный запрос.
- Данные (статусы, идентификаторы систем) — не PII напрямую, но
  `concept_knowledge_exchange_act` может содержать `rrel_payload` с
  реальной подзадачей пользователя.

## Метрики

| Метрика | Значение / статус |
|---|---|
| Latency | Синхронно, без I/O; линейно по числу прямых зависимостей подзадачи (обычно единицы). |
| Accuracy | Не применимо — булева проверка условий, не предсказание; корректность проверена тестами на обоих исходах (готова/не готова). |
| Cost | Пренебрежимо мал. |
| Алерты | Не реализованы. Уместен был бы алерт на подзадачу, годами остающуюся `proposed` из-за незавершённой зависимости (потенциальный дедлок в графе). |

## Тесты

Файл: `tests/test_agent_dispatch_ready_subtask.cpp` (3 теста, все проходят; сценарий 1 дополнительно проверен на живом sc-server).

**1. `DispatchesSubtaskWithNoDependencies`**
Вход: подзадача назначена системе, статус `proposed`, зависимостей нет.
Выход: индикатор `1.0`, статус сменился на `in_progress` (и остался
единственным), создан `concept_knowledge_exchange_act` с
`rrel_receiver` = назначенная система.

**2. `DoesNotDispatchWhenDependencyIncomplete`**
Вход: та же подзадача, но с одной зависимостью в статусе `in_progress`
(не `completed`).
Выход: индикатор `0.0`, статус подзадачи остался `proposed` — не
изменился.

**3. `FinishesWithErrorOnMissingArguments`**
Вход: действие без аргументов.
Выход: `FinishedWithError`.
