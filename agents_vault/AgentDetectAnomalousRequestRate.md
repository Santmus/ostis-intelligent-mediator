---
type: agent
tags: [agent, sc-agent, security, threshold]
action_class: action_detect_anomalous_request_rate
version: (текущий коммит ostis_ps_user_ecosystem)
status: реализован, покрыт тестами, проверен на живом sc-server
---

# AgentDetectAnomalousRequestRate

← [[00 Индекс]] · из [[Кандидаты на реализацию]] (топ-5, #2)

> [!note] k_sigma — аргумент, а не константа
> В отличие от α/β/γ/δ (все явно «типично 0.10» в тексте правил
> [[AgentUpdateTrustReliability|trust-агентов]]), у k_sigma нет
> текстового значения по умолчанию — только наблюдение «27×» в
> иллюстративном сценарии, которое описывает *конкретный инцидент*, а
> не порог. Поэтому здесь k_sigma — третий аргумент действия, вызывающая
> сторона решает, что считать аномальным для данного компонента.

## Идентификация

| Поле | Значение |
|---|---|
| Системный идентификатор (sc-IRI) класса действия | `action_detect_anomalous_request_rate` |
| Тип sc-узла класса | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ класс | `AgentDetectAnomalousRequestRate : public ScActionInitiatedAgent` |
| Файлы | `src/agent/agent_detect_anomalous_request_rate.{hpp,cpp}` |
| Роль | Реализует вычислимую часть `rule_detect_anomalous_request_rate` (часть подсистемы безопасности F_rule_sec, без единого выделенного агента-«хозяина» в каталоге `concepts/sc_agents.scs`) |
| Правило БЗ | `rules/rules_security_privacy.scs` → `rule_detect_anomalous_request_rate` |
| Модуль | `TrustModule` (см. [[AgentComputePiMetric]] — тот же аргумент про один модуль на `.dylib`) |

## Назначение

**Проблема.** Всплеск частоты запросов к компоненту экосистемы может
означать DoS-атаку или попытку подбора (`concept_incident_denial_of_service`/
`concept_incident_tampering` в `concepts/security_privacy.scs`) — но
«много запросов» само по себе ничего не значит без сравнения с обычной
нагрузкой конкретного компонента.

**Цель.** Сравнить наблюдаемую частоту с базовой линией, умноженной на
пороговый коэффициент, и если превышение обнаружено — зафиксировать это
как индикатор и создать экземпляр `concept_security_incident`.

**Ценность.** Автоматическое, воспроизводимое (не «на глаз») решение
«это уже инцидент или ещё нормальная нагрузка» — первый шаг цепочки
реагирования, описанной в `scenario_security_breach_response.scs`.

**Стейкхолдеры:** офицер безопасности экосистемы (`user_security_officer`
в сценарии), `agent_coalition_reconfigurator` (не реализован — должен
был бы реагировать на инцидент), пользователи атакуемого компонента.

## Компетенция

**Capabilities:**
- по трём числам (`rate`, `baseline`, `k_sigma`) вычислить, превышена
  ли базовая линия с заданным запасом;
- записать булев индикатор (1.0/0.0) в sc-ссылку узла-приёмника,
  создав её при первом обращении;
- при обнаружении аномалии — сгенерировать экземпляр
  `concept_security_incident` и связать его с узлом-приёмником;
- корректно завершиться с ошибкой при отсутствии аргументов или
  нечисловом содержимом.

**Limitations (по коду):**
- **не выбирает конкретный STRIDE-подкласс** инцидента (DoS или
  Tampering) — создаёт только базовый `concept_security_incident`;
  критерий выбора подкласса в тексте правила не формализован.
- **не активирует** `agent_coalition_reconfigurator` и
  `agent_provenance_registrar` «для фиксации инцидента», как требует
  текст правила — оба вне зоны ответственности этого агента (первый не
  реализован вовсе; [[AgentComputePiMetric]] реализует лишь часть
  второго, и не в форме, которую можно было бы вызвать отсюда
  напрямую).
- связь инцидент → узел-приёмник — **не именованное отношение
  онтологии**, а обычная `ConstPermPosArc` для трассируемости (по
  аналогии с тем, как `trust_level` хранит свой `value_link`); нет
  формальной связи инцидента с конкретным `target_component`, который
  подвергся атаке — в онтологии нет готового отношения для этого.
- k_sigma нигде не валидируется на > 0 — отрицательный k_sigma сделает
  почти любой ненулевой трафик «аномальным».
- `baseline <= 0` — особый случай (см. Поведение): деление на 0 в
  общем виде не определено, агент трактует это как определённое
  решение, а не как ошибку, что может маскировать некорректный
  входной baseline.

## Вход

**Триггер:** дуга `action_initiated -> <экземпляр action_detect_anomalous_request_rate>`.

| № | Роль | Тип | Обязательность | Валидация |
|---|---|---|---|---|
| 1 | `rate_link` | sc-ссылка с наблюдаемой частотой | обязателен | `IsElement` + парсится `std::stof` |
| 2 | `baseline_link` | sc-ссылка с базовой линией | обязателен | `IsElement` + парсится `std::stof` |
| 3 | `k_sigma_link` | sc-ссылка с пороговым множителем | обязателен | `IsElement` + парсится `std::stof` |
| 4 | `result_level` | узел-приёмник индикатора | обязателен | только `IsElement` |

## Выход

Обновлённая/созданная sc-ссылка `value_link` на `result_level` с
`1.0` (аномалия) или `0.0` (норма). Если аномалия — дополнительно
генерируется `concept_security_incident`, связанный с `result_level`
(`ConstPermPosArc`). Успех — `FinishSuccessfully()` +
`FormResult(resultLevel, valueLink)`.

## Связи

**Зависимости:** `common/float_link_io.hpp` (`ReadFloatLinkContent`/
`WriteFloatLinkContent`), `keynodes/security_keynodes.hpp`
(`SecurityKeynodes::concept_security_incident`,
`action_detect_anomalous_request_rate`).

**Читает:** содержимое трёх числовых sc-ссылок.

**Пишет:** `value_link` `result_level`; при аномалии — создаёт новый
узел класса `concept_security_incident`.

**Workflow.** Независим от остальных пяти агентов; концептуально
питает нереализованные `agent_coalition_reconfigurator` (должен
реагировать на созданный инцидент) — связь только по данным
(`concept_security_incident` в общей sc-памяти), не по прямому вызову.

## Поведение

```
IF  любой из четырёх аргументов не sc-элемент
THEN  FinishWithError()

ELSE IF  rate/baseline/k_sigma не парсятся как числа
THEN  FinishWithError()

ELSE
  IF  baseline > 0
  THEN  ratio := rate / baseline
  ELSE IF  rate > 0
  THEN  ratio := +infinity        // трафик из ниоткуда -- аномалия
  ELSE  ratio := 0                // ни трафика, ни истории -- не аномалия

  anomalous := ratio > k_sigma

  IF  у result_level нет value_link
  THEN  создать value_link (ConstNodeLink) + дугу

  записать (anomalous ? 1.0 : 0.0) в value_link

  IF  anomalous
  THEN  создать узел класса concept_security_incident,
        связать result_level -> incident (ConstPermPosArc)

  action.FormResult(result_level, value_link)
  → action.FinishSuccessfully()
```

Пример из диссертации: `rate=5400, baseline=200` → `ratio=27`; при
`k_sigma=5` — `27 > 5` → аномалия, инцидент создан. Число «27»
дословно совпадает с «превышает базовую линию в 27 раз» из
`scenario_security_breach_response.scs`.

**Обработка ошибок/таймауты:** как у остальных агентов проекта —
`DoProgram` синхронный, ожидание на стороне вызывающего кода.

## Безопасность

- Привилегии — как у остальных агентов `TrustModule`: без ACL, полный
  доступ к sc-памяти в рамках процесса `sc-server`.
- **Специфичный риск:** сам факт «превышения базовой линии»
  вычисляется корректно, но заниженный `baseline` или `k_sigma`,
  переданные вызывающей стороной, приведут к ложным срабатываниям
  (или отсутствию их) без какой-либо проверки правдоподобия значений
  агентом — доверие к параметрам целиком на стороне вызывающего кода.
- Создаваемый `concept_security_incident` — по сути публичное
  заявление об инциденте безопасности; агент не проверяет права
  вызывающей стороны на такое заявление (см. общий для проекта
  Limitation — нет аутентификации инициатора действия).
- Данные (частоты запросов) — абстрактные числа, PII не содержат.

## Метрики

| Метрика | Значение / статус |
|---|---|
| Latency | Синхронно, без I/O — сопоставимо с 2-аргументными trust-агентами (константное число операций, не зависит от размера входа). |
| Accuracy | Не применимо к самой формуле; корректность проверена тестами, включая точное воспроизведение примера из диссертации (27×). |
| Cost | Пренебрежимо мал. |
| Алерты | Не реализованы в самом агенте (см. Limitations — активация `agent_coalition_reconfigurator` не выполняется), но `concept_security_incident` остаётся в sc-памяти как персистентный, запрашиваемый сигнал для внешнего мониторинга. |

## Тесты

Файл: `tests/test_agent_detect_anomalous_request_rate.cpp` (3 теста, все проходят; сценарий 1 дополнительно проверен на живом sc-server — получено `1.000000` + создан `concept_security_incident`).

**1. `DetectsAnomalyAndCreatesIncident`**
Вход: `rate=5400, baseline=200, k_sigma=5` (5400/200=27, «превышает
базовую линию в 27 раз» из `scenario_security_breach_response.scs`).
Выход: индикатор `1.0`, создан узел класса `concept_security_incident`,
`FinishedSuccessfully`.

**2. `DoesNotFlagNormalTraffic`**
Вход: `rate=250, baseline=200, k_sigma=5` (250/200=1.25 ≤ 5).
Выход: индикатор `0.0`, инцидент не создан.

**3. `FinishesWithErrorOnMissingArguments`**
Вход: действие без аргументов.
Выход: `FinishedWithError`.
