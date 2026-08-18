---
type: agent
tags: [agent, trust-agent, sc-agent, ema]
action_class: action_update_trust_accuracy
version: 72ba122 (2026-08-18)
status: реализован, покрыт тестами, проверен на живом sc-server
---

# AgentUpdateTrustAccuracy

← [[00 Индекс]] · компонента [[Модель доверия|t_acc]] · паттерн как у [[AgentUpdateTrustReliability]]

## Идентификация

| Поле | Значение |
|---|---|
| Системный идентификатор (sc-IRI) класса действия | `action_update_trust_accuracy` |
| Тип sc-узла класса | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ класс | `AgentUpdateTrustAccuracy : public ScActionInitiatedAgent` |
| Файлы | `src/agent/agent_update_trust_accuracy.{hpp,cpp}` |
| Роль | Исполнитель правила `rule_update_t_acc_on_quality` (π_tr) |
| Правило БЗ | `rules/rules_trust_update.scs` → `rule_update_t_acc_on_quality` |
| Версия | commit `72ba122` |

## Назначение

**Проблема.** `t_rel` ([[AgentUpdateTrustReliability]]) отвечает только
за факт успеха/провала, но не за *качество* успешного результата. Две
системы могут одинаково «не падать», но выдавать разного качества
ответы — это отдельная компонента доверия `t_acc` («достоверность»),
отслеживающая метрики качества на тестовых/исторических данных.

**Цель.** После каждой обработанной подзадачи зафиксировать достигнутую
точность результата `q_res ∈ [0,1]` (в отличие от бинарного ν_rel — это
непрерывная метрика качества) и сгладить накопленную оценку `t_acc` в
её сторону.

**Ценность.** Позволяет `match(σ, s)` различать не просто «работает/не
работает», а «работает и выдаёт качественный результат» — важно там,
где приемлемы только системы выше определённого порога точности
(например, медицинская диагностика, финансовые решения).

**Стейкхолдеры:** те же, что у [[AgentUpdateTrustReliability]], плюс —
в первую очередь домены с жёсткими требованиями к качеству результата
(см. `scenario_medical_diagnostics.scs`, `scenario_financial_decision_support.scs`
в [[ostis_kb_user_ecosystem]]).

## Компетенция

**Capabilities:**
- по паре (узел `trust_level`, sc-ссылка с `q_res`) вычислить
  экспоненциально сглаженное новое значение `t_acc`;
- создать sc-ссылку при первом обращении (нейтральный старт `t_acc = 0`);
- вернуть обновлённые `trust_level`/`value_link` в результате действия;
- корректно завершиться с ошибкой при некорректных аргументах.

**Limitations (по коду, идентично [[AgentUpdateTrustReliability]]):**
- не проверяет принадлежность первого аргумента `concept_trust_level` —
  только `IsElement`;
- не проверяет, что `q_res` действительно ∈ [0,1] — принимает любое
  число, парсящееся `std::stof`;
- не пишет провенанс (какая именно подзадача дала это `q_res`);
- β = 0.10 — константа в коде, не настраиваемая по контексту/системе;
- дополнительно к общим ограничениям: **не определяет метод измерения
  `q_res`** — расчёт точности (по эталону, по разметке, по
  ансамблю-судье и т.д.) полностью вне зоны ответственности агента,
  он лишь принимает уже готовое число.

## Вход

**Триггер:** дуга `action_initiated -> <экземпляр action_update_trust_accuracy>`.

| № | Роль | Тип | Обязательность | Валидация |
|---|---|---|---|---|
| 1 | `trust_level` | `ScAddr` | обязателен | только `IsElement` |
| 2 | `quality_link` | `ScAddr` sc-ссылки с `q_res` | обязателен | `IsElement` + парсится `std::stof` |

**Формат:** `quality_link` — текстовая sc-ссылка, например `"0.9"`.

## Выход

Обновлённая/созданная sc-ссылка `value_link` на `trust_level` с
содержимым `std::to_string(newTAcc)`. Побочный эффект — генерация
`value_link` + связывающей дуги при первом вызове. Успех —
`FinishSuccessfully()` + `FormResult(trustLevel, valueLink)`.

## Связи

Зависимости, чтение/запись — идентичны [[AgentUpdateTrustReliability]]
(общий `common/float_link_io.hpp`, `TrustKeynodes`). Независим от
остальных трёх EMA-агентов; результат читает [[AgentAggregateTrust]].

## Поведение

```
IF  trust_level не sc-элемент  OR  quality_link не sc-элемент
THEN  FinishWithError()

ELSE IF  содержимое quality_link не число
THEN  FinishWithError()

ELSE
  currentTAcc := значение value_link, если есть, ИНАЧЕ 0 (+ создать при отсутствии)
  newTAcc := (1 - β) * currentTAcc + β * q_res       // β = 0.10
  записать newTAcc, FormResult(...), FinishSuccessfully()
```

Обработка ошибок и таймауты — как у [[AgentUpdateTrustReliability]]
(таймаут задаёт вызывающая сторона через `InitiateAndWait`, у самого
агента внутреннего ожидания нет).

## Безопасность

Идентично [[AgentUpdateTrustReliability]]: полные привилегии процесса
`sc-server`, без ACL на уровне агента; тот же риск — произвольный узел
может быть принят как `trust_level` без проверки класса. `q_res` —
абстрактная метрика качества, PII не содержит.

## Метрики

| Метрика | Значение / статус |
|---|---|
| Latency | Та же характеристика, что у [[AgentUpdateTrustReliability]] — синхронно, без I/O. |
| Accuracy | Не применимо (детерминированная формула); корректность проверена тестами. |
| Cost | Пренебрежимо мал. |
| Алерты | Не реализованы. |

## Тесты

Файл: `tests/test_agent_update_trust_accuracy.cpp` (3 теста, все проходят; сценарий 1 дополнительно проверен на живом sc-server).

**1. `UpdatesTAccOnQualityResult`**
Вход: `t_acc = 0.70`, `q_res = 0.90`.
Выход: `t_acc_new = 0.9·0.70 + 0.1·0.90 = 0.72`, `FinishedSuccessfully`.

**2. `CreatesValueLinkWhenMissing`**
Вход: `trust_level` без sc-ссылки, `q_res = 1.0`.
Выход: создаётся ссылка со значением `0.9·0 + 0.1·1.0 = 0.10`.

**3. `FinishesWithErrorOnMissingArguments`**
Вход: действие без аргументов.
Выход: `FinishedWithError`.
