---
type: agent
tags: [agent, trust-agent, sc-agent, ema]
action_class: action_update_trust_transparency
version: 72ba122 (2026-08-18)
status: реализован, покрыт тестами, проверен на живом sc-server
---

# AgentUpdateTrustTransparency

← [[00 Индекс]] · компонента [[Модель доверия|t_tr]] · паттерн как у [[AgentUpdateTrustReliability]]

## Идентификация

| Поле | Значение |
|---|---|
| Системный идентификатор (sc-IRI) класса действия | `action_update_trust_transparency` |
| Тип sc-узла класса | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ класс | `AgentUpdateTrustTransparency : public ScActionInitiatedAgent` |
| Файлы | `src/agent/agent_update_trust_transparency.{hpp,cpp}` |
| Роль | Исполнитель правила `rule_update_t_tr_on_justification` (π_tr) |
| Правило БЗ | `rules/rules_trust_update.scs` → `rule_update_t_tr_on_justification` |
| Версия | commit `72ba122` |

## Назначение

**Проблема.** Объяснимость — не статическое архитектурное свойство
системы («умеет объяснять / не умеет»), а *наблюдаемое поведение*:
насколько часто система реально сопровождает результат обоснованием,
когда его запрашивают. Это отдельная, историческая, эмпирическая
метрика — `t_tr` (см. определение `concept_trust_component_transparency`
в `concepts/trust.scs`: «не подменяет объяснение e в составе
конкретного результата»).

**Цель.** После каждого запроса обоснования зафиксировать, было ли оно
фактически предоставлено (`ν_tr ∈ {0,1}`), и сгладить `t_tr` в его
сторону.

**Ценность.** Позволяет отличать системы, которые декларируют
поддержку объяснений, от систем, которые их *реально* предоставляют по
запросу — важно в доменах, требующих аудируемости решений
(здравоохранение, финансы, право).

**Стейкхолдеры:** пользователи, которым нужно обоснование решения;
аудиторы/регуляторы; оркестраторы, выбирающие между системами при
требовании объяснимости.

## Компетенция

**Capabilities:**
- по паре (узел `trust_level`, sc-ссылка с `ν_tr`) вычислить
  экспоненциально сглаженное новое значение `t_tr`;
- создать sc-ссылку при первом обращении (нейтральный старт `t_tr = 0`);
- вернуть обновлённые `trust_level`/`value_link`;
- корректно завершиться с ошибкой при некорректных аргументах.

**Limitations (по коду, идентично [[AgentUpdateTrustReliability]]):**
- не проверяет принадлежность `trust_level` классу `concept_trust_level`;
- не проверяет диапазон `ν_tr`;
- не пишет провенанс (какой именно запрос обоснования дал этот индикатор);
- δ = 0.10 — константа в коде;
- **не оценивает качество самого обоснования** — только бинарный факт
  «предоставлено / не предоставлено»; пустая отписка и содержательное
  объяснение с точки зрения этого агента неразличимы.

## Вход

**Триггер:** дуга `action_initiated -> <экземпляр action_update_trust_transparency>`.

| № | Роль | Тип | Обязательность | Валидация |
|---|---|---|---|---|
| 1 | `trust_level` | `ScAddr` | обязателен | только `IsElement` |
| 2 | `justification_link` | `ScAddr` sc-ссылки с `ν_tr` | обязателен | `IsElement` + парсится `std::stof` |

**Формат:** текстовая sc-ссылка, `"1.0"` (обоснование предоставлено) или `"0.0"`.

## Выход

Обновлённая/созданная sc-ссылка `value_link` на `trust_level`. Побочный
эффект — генерация `value_link` + дуги при первом вызове. Успех —
`FinishSuccessfully()` + `FormResult(trustLevel, valueLink)`.

## Связи

Идентично остальным EMA-агентам: `common/float_link_io.hpp`,
`TrustKeynodes`. Независим от [[AgentUpdateTrustReliability]] /
[[AgentUpdateTrustAccuracy]] / [[AgentUpdateTrustSecurity]]; результат
читает [[AgentAggregateTrust]].

## Поведение

```
IF  trust_level не sc-элемент  OR  justification_link не sc-элемент
THEN  FinishWithError()

ELSE IF  содержимое justification_link не число
THEN  FinishWithError()

ELSE
  currentTTr := значение value_link, если есть, ИНАЧЕ 0 (+ создать при отсутствии)
  newTTr := (1 - δ) * currentTTr + δ * ν_tr          // δ = 0.10
  записать newTTr, FormResult(...), FinishSuccessfully()
```

Обработка ошибок/таймауты — как у [[AgentUpdateTrustReliability]].

## Безопасность

Идентично остальным EMA-агентам: полные привилегии процесса
`sc-server`, без проверки класса `trust_level`. `ν_tr` — абстрактный
индикатор, PII не содержит.

## Метрики

| Метрика | Значение / статус |
|---|---|
| Latency | Синхронно, без I/O. |
| Accuracy | Не применимо к формуле; корректность проверена тестами. |
| Cost | Пренебрежимо мал. |
| Алерты | Не реализованы. |

## Тесты

Файл: `tests/test_agent_update_trust_transparency.cpp` (3 теста, все проходят; сценарий 1 дополнительно проверен на живом sc-server).

**1. `UpdatesTTrOnJustificationProvided`**
Вход: `t_tr = 0.60`, `ν_tr = 1` (обоснование предоставлено).
Выход: `t_tr_new = 0.9·0.60 + 0.1·1.0 = 0.64`, `FinishedSuccessfully`.

**2. `CreatesValueLinkWhenMissing`**
Вход: `trust_level` без sc-ссылки, `ν_tr = 1`.
Выход: создаётся ссылка со значением `0.9·0 + 0.1·1 = 0.10`.

**3. `FinishesWithErrorOnMissingArguments`**
Вход: действие без аргументов.
Выход: `FinishedWithError`.
