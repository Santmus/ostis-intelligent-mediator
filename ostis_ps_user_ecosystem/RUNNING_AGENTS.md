# Как запустить и проверить агентов лично

Два независимых способа. Первый — быстрый автоматический прогон на
синтетических данных. Второй — ручной запуск каждого агента на живой
платформе, с наблюдаемым результатом на реальной БЗ.

## 1. Автоматически — `ctest`

Проверяет корректность C++-логики всех 27 агентов + 3 интеграционных
сценария + 1 сквозной тест на реальной KB (106 тестов). Данные для
каждого теста агент собирает сам программно — живую БЗ вы не видите.

```bash
cd ostis_ps_user_ecosystem
cmake --build build
ctest --test-dir build --output-on-failure
```

Ожидаемый результат: `100% tests passed, 0 tests failed out of 106`.

## 2. Вручную, на живой платформе — `scripts/run_agent.py`

Скрипт написан именно для ручной проверки без C++ и без браузерной
консоли: подключается к реально запущенному `sc-server`, инициирует
действие агента (`action_initiated`) на минимальной sc-конструкции,
ждёт результата, сверяет с ожидаемым числом и печатает `OK`/`MISMATCH`.

### Шаг 1 — поднять платформу

```bash
cd /Users/evgeniy_kazacenko/Наука/OSTIS/ostis-web-platform
./start.sh
```

Обычный запуск собирает KB, только если её ещё нет. После любых правок
в `ostis_kb_user_ecosystem/` нужна пересборка:

```bash
./start.sh --rebuild
```

Поднимает `sc-server` на `ws://localhost:8090/ws_json` и веб-интерфейс
`sc-web` на `http://localhost:8000`.

### Шаг 2 — запустить демо

```bash
cd ostis_ps_user_ecosystem
/Users/evgeniy_kazacenko/Наука/OSTIS/ostis-web-platform/interface/sc-web/.venv/bin/python3 \
  scripts/run_agent.py
```

Без аргументов — прогоняет демо для всех 27 агентов по очереди. Можно
указать конкретные имена через пробел:

```bash
.../python3 scripts/run_agent.py aggregate_trust resolve_by_trust_weighted
```

Полный список имён (совпадает с `DEMOS` в конце `scripts/run_agent.py`):

```
update_trust_reliability      update_trust_accuracy
update_trust_security         update_trust_transparency
aggregate_trust                compute_pi_metric
validate_cardinality_constraint  detect_anomalous_request_rate
resolve_by_trust_weighted     dispatch_ready_subtask
register_partial_result       propagate_to_dependent
build_explanation             validate_integration
validate_provenance_chain     exclude_from_coalition
reassign_on_resource_exhaustion  check_acyclicity
validate_right_to_explanation evaluate_progressive_disclosure
choose_modality                record_prov_entity
record_prov_activity          handle_no_admissible_assignment
evaluate_context_aware_layout link_explanation_to_provenance
record_state_transition
```

### Как читать вывод

```
[OK] action_update_trust_reliability: t_rel=0.520
[MISMATCH] action_aggregate_trust: expected=0.560 got=0.480
```

`OK` — агент создал/изменил ровно то, что ожидалось. `MISMATCH` —
печатает ожидаемое и фактическое значение для разбора.

### Безопасность повторных запусков

Каждый прогон:
1. Создаёт свою собственную мини-конструкцию с нуля (те же
   иллюстративные числа, что в `tests/test_agent_*.cpp`) — не трогает
   ни один именованный узел реальной БЗ.
2. После проверки стирает всё, что сам создал.
3. **Не стирает** сам узел действия (`action_addr`) — это осознанно.

> [!warning] Известный баг sc-machine: не стирайте узел действия после InitiateAndWait
> Стирание sc-узла уже завершённого действия (после того как у него
> появилась дуга `action_finished_successfully`/...) через
> `erase_elements` надёжно роняет весь `sc-server` — необработанное
> исключение `utils::ExceptionInvalidParams` в C++
> (`libc++abi: terminating due to uncaught exception`), процесс
> sc-server завершается. Воспроизведено многократно при подготовке
> `run_agent.py`, стандартной последовательностью запросов клиента,
> без единой строки кода агентов TrustModule — похоже на баг в самом
> sc-machine, не в этом проекте. Поэтому `run_agent.py` узел действия
> не стирает — он остаётся в БЗ как несколько «мусорных» sc-элементов
> за прогон. Используйте тот же приём в любом другом скрипте/клиенте,
> вызывающем этих агентов извне C++-тестов.

Повторный запуск сколько угодно раз — безопасен (кроме этого
накапливающегося маленького мусора).

### Если для агента нет готового демо

Контракт аргументов (что означает каждый позиционный `rrel_N`) прямо
расписан в докстринге `src/agent/agent_<имя>.hpp` для каждого агента —
ориентируйтесь на него и на docstring `run_action` в `run_agent.py`.

### Посмотреть глазами

Пока `sc-server` поднят, откройте `http://localhost:8000` (sc-web) —
там можно найти сами формальные объявления агентов
(`sc_agent_of_<имя>` в `ostis_kb_user_ecosystem/concepts/sc_agent_realizations.scs`)
и любые узлы реальной БЗ напрямую.

## Известные ограничения

- Демо для `AgentEvaluateContextAwareLayout`, `AgentLinkExplanationToProvenance`
  и `AgentRecordStateTransition` (все добавлены 2026-09-03) проверены
  только статически (`py_compile`/AST) — не против живого `sc-server`
  (на момент добавления `record_state_transition` сервер уже был поднят
  4+ часа с более старой сборкой KB/агентов; перезапуск с `--rebuild`
  сознательно не выполнялся автоматически, см. текст выше про
  `./start.sh --rebuild`). При первом запуске этих трёх стоит проверить
  вывод внимательнее.
- `run_agent.py` требует venv `sc-web` (пакет `sc_client`) — отдельно
  ставить не нужно, он уже входит в `ostis-web-platform/interface/sc-web/.venv`.
