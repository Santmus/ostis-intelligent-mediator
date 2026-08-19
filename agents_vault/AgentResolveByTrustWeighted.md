---
type: agent
tags: [agent, sc-agent, integration, argmax]
action_class: action_resolve_by_trust_weighted
version: (текущий коммит ostis_ps_user_ecosystem)
status: реализован, покрыт тестами, проверен на живом sc-server
---

# AgentResolveByTrustWeighted

← [[00 Индекс]] · из [[Кандидаты на реализацию]] (топ-5, #3) · переиспользует [[Модель доверия|trust(s,τ,h)]]

> [!note] Единственный агент, читающий уже готовый trust
> Все остальные шесть агентов либо *вычисляют* компоненту доверия, либо
> вообще о доверии не знают. Этот — единственный, который **потребляет**
> уже посчитанный `trust(s,τ,h)` ([[AgentAggregateTrust]]) как готовый
> входной сигнал для другого решения (выбора победителя среди
> конфликтующих результатов).

## Идентификация

| Поле | Значение |
|---|---|
| Системный идентификатор (sc-IRI) класса действия | `action_resolve_by_trust_weighted` |
| Тип sc-узла класса | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ класс | `AgentResolveByTrustWeighted : public ScActionInitiatedAgent` |
| Файлы | `src/agent/agent_resolve_by_trust_weighted.{hpp,cpp}` |
| Роль | Реализует `rule_resolve_by_trust_weighted` (часть `agent_result_integrator`/`agent_conflict_detector`, `rules_integration.scs`) |
| Правило БЗ | `rules/rules_integration.scs` → `rule_resolve_by_trust_weighted` |
| Модуль | `TrustModule` (см. [[AgentComputePiMetric]] — тот же аргумент про один модуль на `.dylib`) |

## Назначение

**Проблема.** Несколько интеллектуальных систем могут дать
противоречащие друг другу результаты на одну подзадачу
(`concept_result_conflict` — семантическая несогласованность,
логическое противоречие и т.д., см. `concepts/interactions.scs`). Один
из способов разрешения — не аргументация, а простое голосование по
доверию к источнику.

**Цель.** Среди набора конфликтующих результатов выбрать тот, что
произведён системой с максимальным `trust(s,τ,h)`, и зафиксировать
итог как `concept_integrated_result`, формально разрешающий
конфликт.

**Ценность.** Дешёвая, быстрая (без построения аргументационного
каркаса, как в `rule_resolve_by_argumentation`) эвристика разрешения
конфликтов — и одновременно замыкает цикл: доверие, накопленное
[[AgentUpdateTrustReliability|EMA-агентами]] и свёрнутое
[[AgentAggregateTrust|агрегатором]], реально влияет на то, чей
результат система в итоге предпочтёт.

**Стейкхолдеры:** `agent_result_integrator`/`agent_conflict_detector`
(не реализованы целиком, только эта часть), конечные пользователи,
получающие один согласованный ответ вместо противоречивых.

## Компетенция

**Capabilities:**
- для каждого кандидата из заданной группы найти производящую систему
  (`nrel_produced_by`) и её уровень доверия (`nrel_has_trust_level` →
  `value_link`);
- выбрать кандидата с максимальным весом (`arg max_i w_i`);
- создать `concept_integrated_result`, связать его с победителем через
  `nrel_integrated_in` и с переданным конфликтом через
  `nrel_resolves_conflict`;
- корректно завершиться с ошибкой при отсутствии аргументов или
  пустой группе кандидатов.

**Limitations (по коду):**
- **не формирует текстовое обоснование** («с обоснованием» в тексте
  правила) — это по сути задача `agent_explanation_builder`, не
  реализованного в проекте; в логе остаётся только отладочное
  сообщение с итоговым весом, не формальная sc-структура.
- **кандидат без доверительной цепочки получает вес 0**, а не
  ошибку/исключение — неотличим от системы с реально нулевым доверием
  (тот же принцип, что и в [[AgentAggregateTrust]]).
- **не проверяет**, что переданные кандидаты действительно
  конфликтуют друг с другом или что `conflict` ссылается именно на
  них — полностью доверяет вызывающей стороне (нет чтения
  `nrel_conflicts_with`/`concept_result_conflict` содержимого для
  верификации).
- **тай-брейк не специфицирован**: при равенстве весов (включая
  «все нули») побеждает первый обнаруженный в порядке обхода дуг
  группы кандидат — порядок обхода не документирован sc-machine API
  как детерминированный.
- в отличие от метрик-агентов, **пустая группа — ошибка**, а не
  нейтральный результат: выбирать не из чего, поэтому это единственный
  агент проекта, где «пусто» не трактуется как 0.

## Вход

**Триггер:** дуга `action_initiated -> <экземпляр action_resolve_by_trust_weighted>`.

| № | Роль | Тип | Обязательность | Валидация |
|---|---|---|---|---|
| 1 | `candidates_group` | узел с исходящими `ConstPermPosArc` к конфликтующим `concept_processing_result` | обязателен | `IsElement`; пустая группа → ошибка |
| 2 | `conflict` | существующий экземпляр `concept_result_conflict` | обязателен | только `IsElement` |

## Выход

Новый экземпляр `concept_integrated_result` (ir), связанный:
- `r* -[nrel_integrated_in]-> ir` (победивший кандидат),
- `ir -[nrel_resolves_conflict]-> conflict` (переданный аргумент).

`FormResult(ir, r*)`. Побочный эффект — генерация `ir` и обеих связей
(`ConstCommonArc` + классифицирующая `ConstPermPosArc` от отношения, по
стандартному паттерну не-ролевых бинарных отношений в sc-machine).

## Связи

**Зависимости:** `common/float_link_io.hpp` (`ReadFloatLinkContent`),
`keynodes/integration_keynodes.hpp` (`IntegrationKeynodes`).

**Читает:** `nrel_produced_by` каждого кандидата, `nrel_has_trust_level`
производящей системы, `value_link` найденного `trust_level` — тот же
узел, в который пишет [[AgentAggregateTrust]].

**Пишет:** новый `concept_integrated_result` + две связи
(`nrel_integrated_in`, `nrel_resolves_conflict`).

**Workflow.** Единственный агент, формально зависящий от **результата**
[[AgentAggregateTrust]] (через данные — читает тот же `trust_level`,
который наполняет агрегатор), хотя технически не вызывает его и не
проверяет, что trust уже посчитан для нужного τ.

## Поведение

```
IF  candidates_group не sc-элемент  OR  conflict не sc-элемент
THEN  FinishWithError()

ELSE
  best := ∅; best_weight := -∞
  FOR EACH candidate IN candidates_group (по ConstPermPosArc):
    system := TargetOf(candidate, nrel_produced_by)         // 0, если нет
    trust_level := TargetOf(system, nrel_has_trust_level)   // 0, если нет
    w := значение value_link trust_level, ИНАЧЕ 0
    IF  w > best_weight
    THEN  best_weight := w; best := candidate

  IF  best == ∅  (группа была пуста)
  THEN  FinishWithError()

  ELSE
    ir := создать узел класса concept_integrated_result
    связать best -[nrel_integrated_in]-> ir
    связать ir -[nrel_resolves_conflict]-> conflict
    action.FormResult(ir, best)
    → action.FinishSuccessfully()
```

**Обработка ошибок/таймауты:** как у остальных агентов — `DoProgram`
синхронный, ожидание на стороне вызывающего кода через
`InitiateAndWait`.

## Безопасность

- Привилегии — как у остальных агентов `TrustModule`: без ACL, полный
  доступ к sc-памяти в рамках процесса `sc-server`.
- **Специфичный риск:** поскольку кандидат без доверительной цепочки
  тихо получает вес 0, а не отклоняется, специально сконструированный
  «пустой» кандидат никогда не выиграет честно — но и не будет
  исключён из рассмотрения с явным предупреждением; в системе с
  преимущественно недоверенными участниками (все веса ≈0) победитель
  выбирается по недокументированному порядку обхода, что не является
  осмысленным решением, а выглядит как оно.
- Агент не проверяет права вызывающей стороны на разрешение
  конфликта — доверяет любому источнику `action_initiated`.
- Данные (доверие, результаты) — не PII, но `concept_integrated_result`
  может стать основанием для решений, влияющих на пользователей
  (`concept_collective_decision <= nrel_inclusion: concept_integrated_result`).

## Метрики

| Метрика | Значение / статус |
|---|---|
| Latency | Синхронно, без I/O; линейно по числу кандидатов — на каждого до 3 переходов по связям (`nrel_produced_by`, `nrel_has_trust_level`, `value_link`). |
| Accuracy | Не применимо к самой формуле (argmax детерминирован); корректность проверена тестами, включая явную проверку, что побеждает именно наибольший вес, а не порядок добавления. |
| Cost | Пренебрежимо мал при типичных размерах группы конфликтующих результатов (обычно единицы). |
| Алерты | Не реализованы. Уместен был бы алерт на «победитель с весом 0» — сигнал, что ни у одного кандидата нет реальной доверительной истории. |

## Тесты

Файл: `tests/test_agent_resolve_by_trust_weighted.cpp` (3 теста, все проходят; сценарий 1 дополнительно проверен на живом sc-server).

**1. `PicksResultFromHighestTrustSystem`**
Вход: три кандидата с доверием 0.60, 0.95, 0.80 (порядок добавления —
не по возрастанию/убыванию, специально).
Выход: побеждает кандидат с 0.95 (не первый и не последний
добавленный); создан `concept_integrated_result`, связан
`nrel_integrated_in`/`nrel_resolves_conflict`.

**2. `TreatsCandidateWithoutTrustChainAsZero`**
Вход: один кандидат без `nrel_produced_by`, другой с доверием 0.10.
Выход: побеждает кандидат с 0.10 — подтверждает Limitation «нет
цепочки доверия = вес 0», не ошибка.

**3. `FinishesWithErrorOnEmptyCandidatesGroup`**
Вход: пустая группа кандидатов.
Выход: `FinishedWithError` — в отличие от метрик-агентов, пустой вход
здесь не 0, а ошибка.
