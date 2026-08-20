---
type: agent
tags: [agent, sc-agent, integrity, shacl, validation]
action_class: action_validate_cardinality_constraint
version: (текущий коммит ostis_ps_user_ecosystem)
status: реализован, покрыт тестами, проверен на живом sc-server
---

# AgentValidateCardinalityConstraint

← [[00 Индекс]] · из [[Кандидаты на реализацию]] (топ-5, #5, последний) · реализует ядро [[SHACL-shapes|rules_integrity_validation.scs]]

> [!note] Один агент вместо шести
> `rules_integrity_validation.scs` формулирует шесть SHACL-shape'ов
> (`shape_user_request_authorship`, `shape_subtask_status_priority`,
> `shape_result_provenance`, `shape_integrated_result_complete`,
> `shape_is_capability`, `shape_context_completeness`). Все шесть
> механически сводятся к одному и тому же действию: посчитать sc-дуги
> заданного отношения у целевого элемента и сравнить со счётом
> `[minCount, maxCount]`. Вместо шести почти идентичных C++-классов
> реализован один общий агент, параметризованный отношением и
> границами; какая именно комбинация параметров реализует какое
> правило — задокументировано прямо в
> `rules/rules_integrity_validation.scs` рядом с каждым правилом.
> Седьмой shape, `shape_provenance_chain_complete`, требует проверки
> транзитивного пути (`nrel_derived_from` до `concept_user_request`),
> а не подсчёта дуг — не реализован этим агентом.

## Идентификация

| Поле | Значение |
|---|---|
| Системный идентификатор (sc-IRI) класса действия | `action_validate_cardinality_constraint` |
| Тип sc-узла класса | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ класс | `AgentValidateCardinalityConstraint : public ScActionInitiatedAgent` |
| Файлы | `src/agent/agent_validate_cardinality_constraint.{hpp,cpp}` |
| Роль | Реализует механическое ядро `agent_integrity_checker` (`rules_integrity_validation.scs`) |
| Правило БЗ | `rules/rules_integrity_validation.scs` → `rule_validate_authorship_shape`, `rule_validate_subtask_status_shape`, `rule_validate_result_provenance_shape`, `rule_validate_integrated_result_shape` (частично), `rule_validate_is_capability_shape` (частично), `rule_validate_context_completeness` |
| Модуль | `TrustModule` (см. [[AgentComputePiMetric]] — тот же аргумент про один модуль на `.dylib`) |

## Назначение

**Проблема.** Дисциплина целостности БЗ (формулы 4–7,
`concepts/integrity.scs`) требует, чтобы у каждого экземпляра
определённого класса были обязательные связи — автор запроса, статус
подзадачи, провенанс результата и т.д. Без механической проверки такие
требования остаются прозой в `nrel_sc_text_translation`, а не
исполняемым правилом.

**Цель.** Дать один универсальный, параметризуемый строительный блок
"посчитать дуги отношения у элемента и сравнить с границами", из
которого шесть SHACL-shape'ов проекта собираются как частные случаи
(разными вызовами с разными аргументами), не дублируя логику подсчёта
шесть раз.

**Ценность.** SHACL-совместимость, заявленная в дизайне БЗ
(`concepts/integrity.scs`: "Концептуально соотносится с подходом
SHACL"), получает реальную вычислимую реализацию хотя бы для
`minCount`/`maxCount`-части ограничений — самой механической и
наименее неоднозначной части шести из семи shape'ов.

**Стейкхолдеры:** `agent_integrity_checker` (концептуальный "хозяин"
правил в каталоге `concepts/sc_agents.scs`, не реализован отдельным
sc-агентом целиком — эта реализация покрывает только его
кардинальную часть), `agent_coalition_reconfigurator` (потенциальный
потребитель `concept_integrity_violation`, не вызывается этим агентом
напрямую).

## Компетенция

**Capabilities:**
- посчитать sc-дуги одного из трёх паттернов связи для заданного
  отношения у заданного целевого элемента:
  - **исходящее не-ролевое** (`target -[ConstCommonArc]-> ?`,
    классифицируется `relation`) — например `nrel_has_provenance`;
  - **входящее не-ролевое** (`? -[ConstCommonArc]-> target`,
    классифицируется `relation`) — например `nrel_formulates⁻¹`;
  - **исходящее ролевое** (`target -[ConstPermPosArc]-> ?`,
    классифицируется `relation`) — например `rrel_goal_context`;
- сравнить счёт с `[minCount, maxCount]` (пустая/нечисловая
  `max_count`-ссылка трактуется как отсутствие верхней границы);
- при нарушении создать экземпляр переданного класса нарушения
  (`concept_cardinality_constraint_violation` или
  `concept_existence_constraint_violation` — правила используют
  разные классы для формально одинаковой механики подсчёта) и
  подвесить его под `result_level`;
- корректно завершиться с ошибкой при отсутствии любого из семи
  аргументов или при нераспознанном `mode`.

**Limitations (по коду):**
- **три паттерна связи — не автоматически распознаваемые**, а явно
  переданный параметр `mode`. Ни один из шести shape'ов не
  формализует направление/роль связи структурно — только прозой
  ("property X⁻¹", "property rrel_Y") — поэтому три маркера-константы
  (`concept_cardinality_check_outgoing_non_role` / `_incoming_non_role`
  / `_outgoing_role`) были добавлены вместе с этой реализацией
  (`rules_integrity_validation.scs`), а не взяты готовыми из
  существующей онтологии.
- **не проверяет `qualifiedValueShape`** — часть `shape_integrated_result_complete`
  ("нет неразрешённого конфликта") и `shape_is_capability` ("класс
  target — `concept_capability_profile`") не реализована: это не
  подсчёт дуг, а отдельная проверка содержимого/класса цели.
- **`shape_provenance_chain_complete` не реализован вовсе** — проверка
  транзитивного пути `nrel_derived_from` принципиально другая задача
  (обход графа, а не подсчёт инцидентных дуг).
- **нарушение не связано формальным отношением с `target`** — только
  подвешено под `result_level` (тот же приём, что и
  `concept_security_incident` в [[AgentDetectAnomalousRequestRate]]).
  Онтология проекта не определяет отношение "нарушение относится к
  элементу", а текст правил ограничивается прозой "ссылка на
  ABox-фрагмент" — агент честно этого не изобретает.
- **не различает "нет ни одной дуги" от "относящаяся дуга есть, но не
  того типа"** — просто считает совпадения по точному отношению; если
  вызывающая сторона передаст не то отношение, агент этого не заметит.
- **шесть shape'ов = минимум шесть-восемь отдельных вызовов** (по
  одному на свойство): агент не принимает список свойств за один
  вызов — с одной стороны, простота и однородность интерфейса; с
  другой — оркестрация "весь shape целиком" ложится на вызывающую
  сторону.

## Вход

**Триггер:** дуга `action_initiated -> <экземпляр action_validate_cardinality_constraint>`.

| № | Роль | Тип | Обязательность | Валидация |
|---|---|---|---|---|
| 1 | `target` | существующий sc-элемент | обязателен | только `IsElement` |
| 2 | `relation` | существующее sc-отношение (`nrel_*`/`rrel_*`) | обязателен | только `IsElement` |
| 3 | `mode` | один из `concept_cardinality_check_outgoing_non_role` / `_incoming_non_role` / `_outgoing_role` | обязателен | `IsElement` + сверка с тремя известными константами; иначе → ошибка |
| 4 | `min_count` | числовая sc-ссылка | обязателен | `IsElement`; должна парситься как число, иначе → ошибка |
| 5 | `max_count` | числовая sc-ссылка | обязателен как элемент, но допускает пустое/нечисловое содержимое | `IsElement`; непарсящееся содержимое трактуется как "верхней границы нет", а не ошибка |
| 6 | `violation_class` | существующий `sc_node_class` нарушения | обязателен | только `IsElement` |
| 7 | `result_level` | узел-приёмник индикатора | обязателен | только `IsElement` |

## Выход

Новая/обновлённая sc-ссылка `value_link` на `result_level` с `1.0`
(в границах) или `0.0` (нарушение). Если нарушение — дополнительно:
новый экземпляр `violation_class`, подвешенный под `result_level`
через `ConstPermPosArc` (без отдельной связи с `target`). Успех —
`FinishSuccessfully()` + `FormResult(target, valueLink)`.

## Связи

**Зависимости:** `common/float_link_io.hpp` (`ReadFloatLinkContent`/
`WriteFloatLinkContent` — переиспользованы без изменений, третий раз
подряд), `keynodes/integrity_keynodes.hpp` (`IntegrityKeynodes`, новый
файл).

**Читает:** дуги `relation` у `target` (в одном из трёх паттернов),
содержимое `min_count`/`max_count`.

**Пишет:** `value_link` на `result_level`; при нарушении — новый узел
класса `violation_class`.

**Workflow.** Независим от остальных девяти агентов проекта по
данным — не читает и не пишет доверие/провенанс/подзадачи/интеграцию.
Единственный агент, оперирующий доменом «целостность БЗ»
(`concept_integrity_constraint`/`concept_integrity_violation`).

## Поведение

```
IF любой из семи аргументов не sc-элемент
THEN FinishWithError()

ELSE IF mode не один из трёх известных маркеров
THEN FinishWithError()

ELSE IF min_count не парсится как число
THEN FinishWithError()

ELSE
  hasMax := max_count парсится как число
  count  := число дуг relation у target по паттерну mode

  valid := count >= minCount AND (NOT hasMax OR count <= maxCount)

  IF NOT valid
  THEN
    создать узел класса violation_class
    подвесить его под result_level (ConstPermPosArc)

  записать (valid ? 1.0 : 0.0) в value_link result_level (создать при отсутствии)
  action.FormResult(target, value_link)
  → action.FinishSuccessfully()
```

**Обработка ошибок/таймауты:** как у остальных агентов — `DoProgram`
синхронный, ожидание на стороне вызывающего кода через
`InitiateAndWait`.

## Безопасность

- Привилегии — как у остальных агентов `TrustModule`: без ACL, полный
  доступ к sc-памяти в рамках процесса `sc-server`.
- **Специфичный риск:** агент доверяет вызывающей стороне в выборе
  `relation`/`mode`/`violation_class` — можно вызвать его с
  несоответствующей друг другу тройкой (например, ролевым отношением
  в режиме `outgoing_non_role`) и получить количество 0 без сигнала об
  ошибке, что будет выглядеть как обоснованное нарушение, а на деле —
  ошибка вызывающей стороны.
- Не проверяется, что `target` действительно принадлежит классу,
  который данный shape предполагает (`targetClass` из прозы правил) —
  агент проверит кардинальность отношения у любого переданного узла.
- Данные (счётчики дуг, классы нарушений) — не PII.

## Метрики

| Метрика | Значение / статус |
|---|---|
| Latency | Синхронно, без I/O; линейно по числу инцидентных дуг заданного отношения у `target` (обычно единицы). |
| Accuracy | Не применимо — детерминированное сравнение счёта с границами; корректность проверена тестами на обоих паттернах (не-ролевой исходящий/входящий) и обоих исходах (в границах/нарушение). |
| Cost | Пренебрежимо мал. |
| Алерты | Не реализованы. Уместен был бы алерт на серию нарушений одного `violation_class` за короткий период — сигнал систематической проблемы генерации данных, а не единичного случая. |

## Тесты

Файл: `tests/test_agent_validate_cardinality_constraint.cpp` (3 теста,
все проходят; все три сценария дополнительно проверены на живом
sc-server реальными данными онтологии).

**1. `ValidatesWhenCountWithinBounds`**
Воспроизводит `shape_result_provenance`: три исходящие связи одного
отношения, `minCount=1`, без `maxCount`.
Выход: индикатор `1.0`, нарушение не создано.
На живом сервере — `shape_context_completeness` на реальном
`interaction_context_001` (`rrel_goal_context`, из
`scenario_request_processing.scs`, где заданы все 5 подконтекстов):
индикатор `1.0`, без нарушения.

**2. `CreatesViolationWhenCountBelowMinimum`**
Воспроизводит `shape_user_request_authorship`: `nrel_formulates⁻¹` без
единого входящего пользователя, `minCount=1, maxCount=1`.
Выход: индикатор `0.0`, создан узел класса
`concept_cardinality_constraint_violation`, подвешенный под
`result_level`.
На живом сервере — тот же shape на реальном `request_001`
(в БЗ ровно один `user_alice => nrel_formulates: request_001`):
индикатор `1.0` (в границах `[1,1]`), без нарушения; отдельно
проверен нарушающий случай на изолированном узле без единой
`nrel_generated_in_context` (`shape_result_provenance`) — индикатор
`0.0`, нарушение класса `concept_existence_constraint_violation`
создано корректно.

**3. `FinishesWithErrorOnMissingArguments`**
Вход: действие без аргументов.
Выход: `FinishedWithError`.
