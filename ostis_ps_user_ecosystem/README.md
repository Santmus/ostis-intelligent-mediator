# ostis_ps_user_ecosystem

Программный слой (sc-агенты на C++) для правил обновления доверия π_tr,
описанных в базе знаний [`ostis_kb_user_ecosystem`](../ostis_kb_user_ecosystem)
(см. `rules/rules_trust_update.scs`, `concepts/trust.scs`). Онтология
объявляет *что* нужно делать; этот код — исполняемая реализация *как*.

## Модель доверия

Каждая интеллектуальная система `s` в экосистеме характеризуется
вектором доверия

```
T(s, τ) = ⟨t_rel(s,τ), t_acc(s,τ), t_sec(s,τ), t_tr(s,τ)⟩
```

где τ — контекст (тип задачи), а компоненты — числа в диапазоне [0, 1]:

| Компонента | Смысл | Обновляется по |
|---|---|---|
| `t_rel` — надёжность | история доступности и корректности ответов | индикатору успешности взаимодействия ν_rel ∈ {0,1} |
| `t_acc` — достоверность | качество на тестовых/исторических данных | достигнутой точности результата q_res ∈ [0,1] |
| `t_sec` — безопасность | соответствие политикам доступа, частота инцидентов | индикатору отсутствия инцидента ν_sec ∈ {0,1} |
| `t_tr` — прозрачность | частота предоставления обоснования по запросу | индикатору предоставления обоснования ν_tr ∈ {0,1} |

Скалярная агрегированная оценка `trust(s, τ, h) = Σ λ_k · t_k` (Σλ_k = 1)
используется функцией подбора исполнителя `match(σ, s)` при распределении
подзадач между системами сообщества.

## Агенты

Модуль `TrustModule` ([src/module/trust_module.cpp](src/module/trust_module.cpp))
регистрирует пять sc-агентов. Каждый реагирует на инициацию действия
своего класса (дуга `action_initiated -> <действие>`) — см.
[«Как запускать» ниже](#как-запускать).

### 1–4. Обновление одной компоненты (экспоненциальное сглаживание)

Четыре агента реализованы одним и тем же паттерном:

```
t_k(s, τ) ← (1 − коэфф.) · t_k(s, τ) + коэфф. · индикатор
```

| Агент | Класс действия | Правило БЗ | Коэфф. (по умолч.) | 2-й аргумент |
|---|---|---|---|---|
| [AgentUpdateTrustReliability](src/agent/agent_update_trust_reliability.cpp) | `action_update_trust_reliability` | `rule_update_t_rel_on_outcome` | α = 0.10 | ν_rel |
| [AgentUpdateTrustAccuracy](src/agent/agent_update_trust_accuracy.cpp) | `action_update_trust_accuracy` | `rule_update_t_acc_on_quality` | β = 0.10 | q_res |
| [AgentUpdateTrustSecurity](src/agent/agent_update_trust_security.cpp) | `action_update_trust_security` | `rule_update_t_sec_on_incident` | γ = 0.10 | ν_sec |
| [AgentUpdateTrustTransparency](src/agent/agent_update_trust_transparency.cpp) | `action_update_trust_transparency` | `rule_update_t_tr_on_justification` | δ = 0.10 | ν_tr |

**Аргументы действия (одинаковые для всех четырёх):**
1. `trust_level` — существующий экземпляр `concept_trust_level`, несущий
   текущее значение компоненты в виде числовой sc-ссылки
   (`trust_level -> value_link`, `ConstPermPosArc`).
2. индикатор/значение — числовая sc-ссылка с новым наблюдением.

Если у `trust_level` ещё нет числовой sc-ссылки, агент создаёт её и
принимает начальное значение `0` (нейтральный старт — «новая система без
истории»). Каждый вызов — один шаг экспоненциального сглаживания:
недавние наблюдения весят больше, но резкий единичный сбой не обнуляет
накопленную репутацию.

Смысл коэффициента 0.10 — компромисс между инерцией (устойчивость к
шуму/единичным аномалиям) и чувствительностью (система с реальной
деградацией качества теряет доверие за разумное число взаимодействий, а
не мгновенно и не бесконечно долго).

### 5. Агрегация вектора в скаляр

[AgentAggregateTrust](src/agent/agent_aggregate_trust.cpp) —
`action_aggregate_trust`, правило `rule_aggregate_trust`:

```
trust(s, τ, h) = λ_rel·t_rel + λ_acc·t_acc + λ_sec·t_sec + λ_tr·t_tr
```

Веса по умолчанию (иллюстративный пример из `scenario_trust_assessment.scs`):
`λ = (0.40, 0.25, 0.15, 0.20)`, Σλ = 1. Правило допускает и обучаемые веса
(«либо обучаются» в тексте `rules_trust_update.scs`) — это отдельная, пока
не реализованная задача; сейчас веса — фиксированные константы в коде.

В отличие от первых четырёх агентов, это **не** накопление во времени:
результат пересчитывается заново из текущих значений компонент при
каждом вызове, а не смешивается со старым `trust`.

**Аргументы действия (5 штук, `rrel_1`…`rrel_5`):**
1–4. Экземпляры `concept_trust_level` компонент `t_rel`, `t_acc`, `t_sec`,
`t_tr` (в этом порядке). Компонента без числовой sc-ссылки трактуется
как `t_k = 0`, а не как ошибка.
5. `trust_level` — узел для итоговой скалярной оценки; sc-ссылка
   создаётся при отсутствии, как и у первых четырёх агентов.

## Архитектура

- **Хранение значений.** Числа хранятся как sc-ссылки с **текстовым**
  содержимым (`"0.838000"`), а не сырыми байтами float — потому что
  `ScAgentContext::SetLinkContent`/`GetLinkContent` для числовых типов
  используют несовместимые пути сериализации (см. комментарий в
  [common/float_link_io.hpp](src/common/float_link_io.hpp)). Все пять
  агентов используют общие `ReadFloatLinkContent`/`WriteFloatLinkContent`
  из этого файла.
- **Ключевые sc-элементы** — [src/keynodes/trust_keynodes.hpp](src/keynodes/trust_keynodes.hpp).
  `concept_trust_level` — уже существующий класс онтологии (общий
  контейнер для значения любой компоненты); `action_*` — классы действий,
  объявленные и в C++ (для программной регистрации), и в SC-коде
  (`rules/rules_trust_update.scs`, для документации/единообразия модели).
- **Обработка ошибок.** Отсутствие требуемых аргументов у действия —
  `action.FinishWithError()`, а не тихий пропуск; см. тесты
  `*FinishesWithErrorOnMissingArguments`.

## Сборка и тесты

```sh
cd ostis_ps_user_ecosystem
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
DYLD_LIBRARY_PATH="../../sc-machine/build/Release/lib" ./ostis-ps-trust-tests   # macOS
# LD_LIBRARY_PATH=... на Linux
```

15 тестов (3 на агента: основной сценарий, создание sc-ссылки при
отсутствии, ошибка при пустых аргументах).

В составе платформы `ostis-web-platform` сборка встроена в `setup.sh`
(этап «ostis_ps_user_ecosystem»), а расширение подключается к
`sc-server` через `start.sh` (`EXTENSIONS=...;ostis_ps_user_ecosystem/build/extensions`).

## Как запускать

Агенты не запускаются напрямую — это sc-агенты, реагирующие на события в
базе знаний (создание действия своего класса + дуга `action_initiated`).
Два практических пути:

1. **Из другого агента/сценария** (штатный путь) — любой код, вычисливший
   индикатор по итогам взаимодействия, вызывает
   `action.SetArguments(...); action.Initiate();` на C++.
2. **Вручную/для тестов** — CLI [scripts/update_trust.py](scripts/update_trust.py)
   поверх `py-sc-client` (websocket-протокол sc-server, тот же, что
   использует sc-web):

   ```sh
   source ../interface/sc-web/.venv/bin/activate   # относительно ostis-web-platform
   python3 scripts/update_trust.py --component rel --trust-level trust_level_my_service --value 1.0
   ```

   `--component` — `rel|acc|sec|tr`; `--trust-level` — системный
   идентификатор узла (создаётся автоматически, если не существует);
   `--value` — индикатор/значение второго аргумента. Агрегация (`agg`)
   через этот CLI пока не поддержана — нужно 5 аргументов вместо 2, для
   неё сейчас нужен собственный вызов через `py-sc-client`/`ts-sc-client`
   по образцу пятиаргументных сценариев в тестах.
