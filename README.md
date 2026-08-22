# ostis-intelligent-mediator

Код к диссертационной работе **«Модели и методы взаимодействия
пользователей с сообществами интеллектуальных систем»** (Казаченко Е. А.,
БГУИР, научная школа OSTIS, рук. Шункевич Д. В.).

Пользователь сегодня обращается не к одной интеллектуальной системе, а к
их сообществу — и вынужден сам решать, кому адресовать задачу, как
совместить разнородные ответы и кому верить, когда они противоречат друг
другу. Работа выносит эту координацию в отдельный слой —
интеллектуального посредника, который принимает цель на языке предметной
области, а не команду на языке интерфейса. В отличие от классических
посреднических решений (Видерхольд и др.), где медиируются данные и их
представления, здесь медиируются намерение пользователя и компетенции
исполнителей, а состав участников остаётся открытым.

Этот репозиторий — операционализация части формальных правил
диссертации на платформе [OSTIS](https://github.com/ostis-ai): онтология
посредника на языке SCs и модуль C++ sc-агентов поверх
[sc-machine](https://github.com/ostis-ai/sc-machine).

## Структура репозитория

```
.
├── ostis_kb_user_ecosystem/   # онтология (SCs): концепты, отношения, правила, аксиомы, сценарии
├── ostis_ps_user_ecosystem/   # программная система (C++): модуль sc-агентов TrustModule + тесты
└── agents_vault/              # Obsidian-vault: онтологическая документация по каждому агенту
```

### `ostis_kb_user_ecosystem/` — база знаний

SCs-исходники, предназначенные для сборки через `sc-builder` в составе
`ostis-web-platform`:

- `concepts/` — базовые понятия (в т.ч. `concept_sc_agent`, каталог из
  12 агентов, задуманных диссертацией);
- `relations/`, `definitions/`, `axioms/` — отношения и формальный
  аппарат онтологии;
- `rules/` — формальные правила по каждому агенту (например,
  `rules_trust_update.scs`, `rules_cooperation.scs`,
  `rules_integrity_validation.scs`);
- `scenarios/`, `examples/` — иллюстративные сценарии, используемые как
  контрольные примеры в тестах;
- `tests/` — SCs-тесты онтологии.

### `ostis_ps_user_ecosystem/` — программная система

CMake-проект на C++17, собираемый против `sc-machine`. Все агенты живут
в одном модуле `TrustModule` (`SC_MODULE_REGISTER`) и одной разделяемой
библиотеке `ostis-ps-trust`, независимо от предметной области:

- `src/agent/` — реализация sc-агентов (`ScActionInitiatedAgent`);
- `src/keynodes/` — типизированные обёртки над sc-элементами по доменам;
- `src/common/` — переиспользуемые утилиты чтения/записи sc-памяти;
- `src/module/` — регистрация `TrustModule`;
- `tests/` — GTest-тесты, включая сквозные интеграционные сценарии;
- `scripts/run_agent.py` — запуск любого агента на живом `sc-server`
  через `py-sc-client`, без сборки C++-тестов.

### `agents_vault/` — онтологическая документация

Obsidian-vault: по одной заметке на агент (Идентификация → Назначение →
Компетенция → Вход → Выход → Связи → Поведение → Безопасность →
Метрики → Тесты) плюс индекс [`00 Индекс.md`](agents_vault/00%20Индекс.md)
и обзор нереализованных кандидатов
[`Кандидаты на реализацию.md`](agents_vault/Кандидаты%20на%20реализацию.md).

## Реализованные агенты

12 sc-агентов модуля `TrustModule`, операционализирующих 3 из 12 файлов
правил каталога `concepts/sc_agents.scs`:

| Группа | Агенты | Правила |
|---|---|---|
| `agent_trust_evaluator` (доверие, полностью) | `AgentUpdateTrustReliability`, `AgentUpdateTrustAccuracy`, `AgentUpdateTrustSecurity`, `AgentUpdateTrustTransparency`, `AgentAggregateTrust` | `rules_trust_update.scs` |
| `agent_cooperative_executor` (кооперация, полностью) | `AgentDispatchReadySubtask`, `AgentRegisterPartialResult`, `AgentPropagateToDependent` | `rules_cooperation.scs` |
| Прочие | `AgentComputePiMetric`, `AgentDetectAnomalousRequestRate`, `AgentResolveByTrustWeighted`, `AgentValidateCardinalityConstraint` | `rules_provenance_completion.scs` (частично), `rules_security_privacy.scs`, `rules_integration.scs` (частично), `rules_integrity_validation.scs` (частично) |

Подробная карта агентов, зависимостей и сквозных тестов —
в [`agents_vault/00 Индекс.md`](agents_vault/00%20Индекс.md).

## Сборка и тесты

Требуется собранный рядом [sc-machine](https://github.com/ostis-ai/sc-machine)
(`../sc-machine/build/Release` по умолчанию, переопределяется через
`SC_MACHINE_BUILD_DIR`) и GTest.

```bash
cmake -S ostis_ps_user_ecosystem -B ostis_ps_user_ecosystem/build
cmake --build ostis_ps_user_ecosystem/build
ctest --test-dir ostis_ps_user_ecosystem/build
```

Для запуска агента на живом `sc-server` без пересборки тестов —
`ostis_ps_user_ecosystem/scripts/run_agent.py` (см. docstring файла).

## Дальнейшие шаги

Обзор оставшихся нереализованных агентов/правил с оценкой готовности
каждого (ГОТОВО / ЧАСТИЧНО / НЕТ) — в
[`agents_vault/Кандидаты на реализацию.md`](agents_vault/Кандидаты%20на%20реализацию.md).
