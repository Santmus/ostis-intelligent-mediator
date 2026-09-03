# Аудит SC-агентов интеллектуального посредника

Дата первого аудита: 28 августа 2026 г.  
Дата переструктурирования: 01 сентября 2026 г. — документ реорганизован по
статусу (сделано / сделано частично / осталось сделать), а не по
хронологии находок, чтобы не смешивать констатацию проблемы с её решением
в одном потоке текста.  
Дата предыдущей редакции: 01 сентября 2026 г. — добавлен `AgentBuildExplanation`
(§5.6) и `AgentValidateIntegration` (§5.7), второй и третий агенты из
таблицы §7.5 «Отсутствующие агенты», перенесённые в реализованные
(частично); `AgentValidateIntegration` закрывает часть заявленной области
`AgentVerifyResult` (explanation + conflicts), опираясь на данные, которые
теперь реально создаёт `AgentBuildExplanation`. Добавлен также
`AgentValidateProvenanceChain` (§5.8) — закрывает `rule_verify_transitive_closure`/
`rule_validate_provenance_chain` (транзитивная достижимость запроса по
`nrel_derived_from`), седьмое из семи правил `rules_integrity_validation.scs`
(файл теперь покрыт полностью). Добавлены также `AgentExcludeFromCoalition`
(§5.9) и `AgentReassignOnResourceExhaustion` (§5.10) — механическая часть
`rule_on_system_failure`/`rule_on_resource_exhaustion`
(`rules_coalition_reconfiguration.scs`), первые агенты семейства
`agent_coalition_reconfigurator`.  
Дата предыдущей редакции: 02 сентября 2026 г. — добавлен `AgentCheckAcyclicity`
(§5.11) — закрывает `rule_check_acyclicity` (`rules_decomposition.scs`),
первый агент семейства `agent_task_decomposer`: DFS-проверка ацикличности
над `nrel_subtask_depends_on`, ограниченная T(q) = decomposed_into(q, ·).
Добавлен также `AgentValidateRightToExplanation` (§5.12) — закрывает
`rule_right_to_explanation` (`rules_security_privacy.scs`), второй агент
файла F_rule_sec наряду с `AgentDetectAnomalousRequestRate`: условный
гейт delivered_to ∧ request_type ⇒ explained_by, переиспользующий уже
существующий класс `concept_property_right_to_explanation` для узла
нарушения вместо объявления нового подкласса. Добавлен также
`AgentEvaluateProgressiveDisclosure` (§5.13) — закрывает
`rule_progressive_disclosure` (`rules_user_adaptation.scs`), первый
агент файла F_rule_adapt: двухпороговое решение по глубине
provenance-цепочки (`nrel_derived_from`) и количеству частичных
результатов. При этом обнаружено, что соседнее правило
`rule_choose_modality`, ранее помеченное READY в
`agents_vault/Implementation Backlog.md`, таковым не является — ни одно
из двух условий ветвления не имеет типизированного представления в
онтологии; оценка готовности в Implementation Backlog.md исправлена.
Затем правило доведено до конца: добавлен `AgentChooseModality` (§5.14) —
закрывает `rule_choose_modality`, второй агент F_rule_adapt наряду с
`AgentEvaluateProgressiveDisclosure`. Потребовалось точечное расширение
онтологии двумя узкими подклассами (`concept_visual_perception_constraint`,
`concept_preference_visual_modality`), формализующими ровно то, что уже
называла проза правила, без изобретения новой семантики.
`rules_user_adaptation.scs` теперь 2 из 6 правил. Добавлены также
`AgentRecordProvEntity` (§5.15) и `AgentRecordProvActivity` (§5.16) —
закрывают `rule_auto_record_prov_entity`/`rule_auto_record_prov_activity`
(`rules_provenance_completion.scs`): заполнение PROV-O триады
(`concept_prov_entity`/`_activity`/`_agent`, `concepts/provenance.scs`),
полностью типизированной, но не заполнявшейся ни одним агентом. В
отличие от `rule_choose_modality`, оба правила действительно были READY
— вся нужная типизация уже существовала. Тем самым снят частный блокер
`rule_link_explanation_to_provenance` (нужен был типизированный
`concept_prov_activity`/`concept_prov_agent` — теперь есть чем их
создавать), само правило по-прежнему не реализовано.
`rules_provenance_completion.scs` теперь 4 из 6 правил. Добавлен также
`AgentHandleNoAdmissibleAssignment` (§5.17) — закрывает
`rule_handle_no_admissible_assignment` (`rules_allocation.scs`), первый
агент семейства `agent_task_allocator` и, по итогам этого обзора,
последнее оставшееся действительно READY и нереализованное правило во
всём `agents_vault/Implementation Backlog.md` — всё, что осталось,
теперь PARTIAL (упирается в неопределённую формулу/таблицу) либо NO
(LLM-задача).  
Далее проведён отдельный проход — онтологическое проектирование
PARTIAL-правил: среди правил, блокированных не открытой формулой, а
узким, называемым пробелом типизации (тот же класс, что у
`rule_choose_modality`), найдено одно подходящее —
`rule_context_aware_layout`. Добавлен `AgentEvaluateContextAwareLayout`
(§5.18) — закрывает гейт (условие «night-shift» ИЛИ «low-light»,
формализовано двумя новыми подклассами `concept_user_situation_context`);
консеквент правила частично остаётся неквантифицированным («снизить
количество элементов» — тот же класс блокировки, что у `cognitive_load(u)`),
поэтому закрыта только механическая часть детектирования, а не весь
консеквент. `rules_user_adaptation.scs` теперь 3 из 6 правил.  
Дата этой редакции: 03 сентября 2026 г. — добавлен
`AgentLinkExplanationToProvenance` (§5.19) — закрывает
`rule_link_explanation_to_provenance` (`rules_provenance_completion.scs`),
последний блокер которого (нетипизированные PROV-O узлы для акта
объяснения) сняли `AgentRecordProvEntity`/`AgentRecordProvActivity`
(§5.15/§5.16) днём ранее. В отличие от всех правил, закрытых в этом
проходе ранее, новой онтологии не потребовалось вовсе — только монтаж
уже существующих отношений (`nrel_prov_was_derived_from`/
`nrel_prov_used`/`nrel_prov_was_attributed_to`) на уже существующие
роли (`rrel_uses_results`/`rrel_applies_rules` с
`explanation_construction_act`, §5.6). `rules_provenance_completion.scs`
теперь 5 из 6 правил — остался только `rule_fill_missing_provenance`
(эвристика без алгоритма в тексте).  
Дата этой редакции (продолжение): 03 сентября 2026 г. — закрыт §7.7,
пункт 1: `tests/test_load_real_kb.cpp` грузит реальную KB
(изолированную build-копию, не продакшен `repo/`) и проверяет, что ни
один keynode проекта не был молча досоздан вместо резолва — см. §7.7 для
разбора, почему прямая проверка `IsElement(...)` для этого не подходит
(`ScKeynode` резолвит по схеме find-OR-CREATE) и как это обойдено
сравнением общего счётчика элементов KB против отдельно собираемого
`tools/kb_baseline_counter.cpp`. Единственный из десяти пунктов §7.7, не
зависящий от отсутствующего агента-оркестратора жизненного цикла (§7.1).  
Дата этой редакции (продолжение): 03 сентября 2026 г. — по итогам
сравнительного изучения `ostis-web-platform`/`ims.ostis.kb` добавлен §4.4:
все 26 агентов формально объявлены по каноническому паттерну OSTIS
(`abstract_sc_agent`/`platform_dependent_abstract_sc_agent`,
`concepts/sc_agent_realizations.scs`) — до этого ни один из них не был
виден платформенным механизмам поиска агентов по онтологии, только через
C++-подписку `ScActionInitiatedAgent`.  
Дата этой редакции (продолжение): 03 сентября 2026 г. — сквозная
механическая проверка онтологии (все `nrel_*`/`rrel_*` объявлены как
отношения; все 94 секции `main.scs` имеют содержимое; все
`concept_*`/`rule_*`/`affirmation_*`/`definition_*`/`axiom_*` объявлены
хотя бы одной из двух форм — subject-старт или target лейбловой дуги)
нашла 4 реальные проблемы: два висячих класса
(`concept_aggregation_rule` в `scenario_explanation_construction.scs`,
не объявлен нигде — заменён на `concept_sc_rule`, базовый класс всех
остальных правил; `concept_trust_for_task` в `example_trust.scs`,
избыточное и ошибочное дополнительное типирование `trust_level_energy`,
который уже корректно типизирован `concept_trust_level` — удалено) и
два неподключённых «осиротевших» блока-черновика
(`section_assignment_example` в `scenario_medical_diagnostics.scs` —
дублировал назначение `subtask_image_segmentation`, но другому
исполнителю (`intelligent_system_image_analyzer`), который даже не
входит в `community_medical_diagnostic`, то есть прямо противоречил
основному сценарию; `section_explanation_example` в
`scenario_explanation_construction.scs` — дублировал
«реифицированный акт построения объяснения» под другими именами
(`explanation_77` вместо `explanation_act_001`)). Оба блока не были
нигде подключены (ни в декомпозиции `main.scs`, ни как
`rrel_key_sc_element` чего-либо), все их идентификаторы больше нигде в
KB не встречались — удалены целиком.  
Дата этой редакции (продолжение): 03 сентября 2026 г. — по итогам
обсуждения дальнейшего плана работы (синтез архитектуры в формате
«заседания комиссии»: оркестрация жизненного цикла, мост к внешним ИС,
сервис онтологического перевода) выбрана и реализована Фаза 1 §7.1:
добавлен `AgentRecordStateTransition` (§5.20) — механический учёт
перехода состояния запроса для уже покрытого агентами участка
конвейера (`x_exec`/`x_aggregate`/`x_verify`, ребро переназначения
`x_exec → x_assign`), проверяющий граф допустимости
`nrel_transitions_to` и ресурсное ограничение `b_rem`
(`constraint_separate_resource_decrease`) и строящий полный пакет
провенанса перехода (`constraint_transition_provenance`). Впервые в
проекте действие операционализирует не `rule_*`-текст, а напрямую
`constraint_*`/`affirmation_*` из `axioms/axioms_constraints.scs` и
`concepts/mediation_lifecycle.scs` — новый
`rules/rules_mediation_orchestration.scs`. §7.1 остаётся открытым в
основном (выбор перехода, оставшиеся 9 состояний, цикл
перепланирования — Фаза 2, не начата), см. §5.20/§7.1.  
Проверено: `ctest` — 106 из 106 тестов пройдены (было 99 — добавлены
6 юнит-тестов `AgentRecordStateTransition` и один новый сквозной
`TEST_F` в `test_integration_request_pipeline.cpp`), сборка KB через
`sc-builder` — чисто, 34741 элемент (было 34579 до Фазы 1); продакшен
`repo/` не тронут — контрольная сумма директории идентична до и после.

## 1. Цель и границы аудита

Проверен проект `ostis_ps_user_ecosystem` на соответствие актуальной концептуальной модели интеллектуального посредника и актуализируемой онтологии `ostis_kb_user_ecosystem`.

Оценивались:

- состав и назначение реализованных SC-агентов;
- соответствие их входов и выходов онтологическим сущностям;
- покрытие жизненного цикла обработки запроса;
- обработка конфликтов, доверия, провенанса, целостности и безопасности;
- качество модульных и интеграционных тестов;
- готовность проекта к непрерывной проверке через CTest.

## 2. Текущее техническое состояние

| Проверка | Результат |
|---|---|
| Сборка `cmake --build build` | Успешно |
| `ctest --test-dir build --output-on-failure` | 106 из 106 тестов пройдены |
| Сборка KB (`sc-builder --clear`) | Успешно, 34741 элемент, без ошибок |

## 3. Состав агентов — сводка

| Агент | Назначение | Статус |
|---|---|---|
| `AgentUpdateTrustReliability` | EMA-обновление `t_rel` | Сделано частично — диапазон `[0,1]` валидируется |
| `AgentUpdateTrustAccuracy` | EMA-обновление `t_acc` | Сделано частично — диапазон `[0,1]` валидируется |
| `AgentUpdateTrustSecurity` | EMA-обновление `t_sec` | Сделано частично — диапазон `[0,1]` валидируется |
| `AgentUpdateTrustTransparency` | EMA-обновление `t_tr` | Сделано частично — диапазон `[0,1]` валидируется |
| `AgentAggregateTrust` | Взвешенная агрегация доверия | Осталось сделать (§7.4) |
| `AgentComputePiMetric` | Расчёт полноты провенанса | Сделано частично (§5.3) |
| `AgentDetectAnomalousRequestRate` | Детектирование аномальной частоты запросов | Сделано частично (§5.5) |
| `AgentResolveByTrustWeighted` | Выбор результата с максимальным доверием | Сделано частично (§5.2) |
| `AgentDispatchReadySubtask` | Перевод готовой подзадачи к исполнению | Осталось сделать (§7.3) |
| `AgentRegisterPartialResult` | Регистрация частичного результата | Сделано частично (§5.1) |
| `AgentPropagateToDependent` | Передача результата зависимой подзадаче | Сделано (§4.3) |
| `AgentValidateCardinalityConstraint` | Кардинальная проверка SC-графа | Сделано частично (§5.4) |
| `AgentBuildExplanation` | Реификация акта построения объяснения | Сделано частично (§5.6) |
| `AgentValidateIntegration` | Валидация интегрированного результата (explanation + conflicts) | Сделано частично (§5.7) |
| `AgentValidateProvenanceChain` | Достижимость запроса по цепочке `nrel_derived_from` | Сделано частично (§5.8) |
| `AgentExcludeFromCoalition` | Исключение отказавшей системы из коалиции | Сделано частично (§5.9) |
| `AgentReassignOnResourceExhaustion` | Массовое переназначение подзадач при истощении ресурса | Сделано частично (§5.10) |
| `AgentCheckAcyclicity` | Проверка ацикличности графа зависимостей подзадач | Сделано частично (§5.11) |
| `AgentValidateRightToExplanation` | Гейт-проверка права пользователя на объяснение | Сделано частично (§5.12) |
| `AgentEvaluateProgressiveDisclosure` | Решение о необходимости прогрессивного раскрытия | Сделано частично (§5.13) |
| `AgentChooseModality` | Выбор модальности взаимодействия | Сделано частично (§5.14) |
| `AgentRecordProvEntity` | Регистрация PROV Entity | Сделано частично (§5.15) |
| `AgentRecordProvActivity` | Регистрация PROV Activity | Сделано частично (§5.16) |
| `AgentHandleNoAdmissibleAssignment` | Обработка отсутствия допустимого назначения | Сделано частично (§5.17) |
| `AgentEvaluateContextAwareLayout` | Детектирование условия контекстно-зависимой компоновки | Сделано частично (§5.18) |
| `AgentLinkExplanationToProvenance` | Связывание акта построения объяснения с провенансом | Сделано (§5.19) |
| `AgentRecordStateTransition` | Механический учёт перехода состояния запроса | Сделано частично (§5.20) |

Модуль `TrustModule` для всех агентов один — реорганизация не начата (§7.6).

## 4. Сделано

Пункты, у которых в исходном тексте аудита не осталось ни одного
именованного открытого хвоста.

### 4.1. Диапазон `[0,1]` для метрик доверия (было: п.6.5)

**Проблема была:** все четыре EMA-агента принимали `quality`/`outcome` без
проверки интервала — `1.5` или отрицательное значение нарушало формальное
условие `t_rel,t_acc,t_sec,t_tr ∈ [0,1]`.

**Сделано:** все четыре агента (`AgentUpdateTrustReliability/Accuracy/
Security/Transparency`) отклоняют значение вне `[0,1]` через
`FinishWithError` (`trust_utils::IsUnitInterval`, `common/float_link_io.hpp`)
— выбрано отклонение, а не нормировка, как и рекомендовал аудит (нормировка
маскировала бы ошибку источника данных).

### 4.2. Регистрация тестов в CTest (было: п.9.3)

**Проблема была:** тестовый бинарник `ostis-ps-trust-tests` собирался и
проходил при прямом запуске (38/38), но не был зарегистрирован через
`add_test(...)`/`gtest_discover_tests(...)` — `ctest` не находил ни одного
теста, то есть типовой прогон в CI не проверял ничего.

**Сделано:** в `CMakeLists.txt` добавлены `include(GoogleTest)` и
`gtest_discover_tests(ostis-ps-trust-tests)`. `ctest --test-dir build
--output-on-failure` находит и проходит 41 из 41 теста.

### 4.3. `AgentPropagateToDependent` — семантика передачи результата (было: п.6.10)

**Проблема была:** агент использовал зависимую подзадачу σ' в роли
`rrel_receiver` коммуникативного акта, хотя подзадача не является
участником коммуникации.

**Сделано:** для каждой найденной σ' безусловно устанавливается
`result -[nrel_is_input_for]-> σ'` (`relations/relations_execution.scs`);
коммуникативный акт (`concept_knowledge_exchange_act`, тип
`concept_act_inform`) создаётся дополнительно и только когда у σ' уже есть
`nrel_assigned_to` — его получателем становится назначенный исполнитель, а
не сама σ'. Если исполнитель ещё не назначен, `is_input_for` остаётся
единственной и достаточной связью, как и требовал аудит. Отдельный класс
`DataTransferAct` из предпочтительной схемы аудита не введён — его роль
закрыта уже существующим `concept_knowledge_exchange_act`.

Файлы: `src/agent/agent_propagate_to_dependent.{hpp,cpp}`,
`tests/test_agent_propagate_to_dependent.cpp` (4 теста),
`agents_vault/AgentPropagateToDependent.md`.

### 4.4. Формальное объявление агентов по паттерну OSTIS (было: не упомянуто в исходном аудите — найдено сравнительным изучением `ostis-web-platform`/`ims.ostis.kb` 03.09.2026)

**Проблема была:** ни один из 26 агентов проекта не был объявлен
принятым в OSTIS каноническим способом — узлом `<- abstract_sc_agent`
со связями `nrel_primary_initiation_condition` (условие первичной
активации по событию), `nrel_initiation_condition_and_result`
(шаблоны условия/результата) и `nrel_sc_agent_key_sc_elements`
(ключевые sc-элементы действия), реализация которого помечена
`<- platform_dependent_abstract_sc_agent`. Проверено по 90+ реальным
образцам в `ims.ostis.kb` (в т.ч.
`.../lib_c_agents/{erase_elements,search_of_all_input_arcs}/*.scs` —
готовые C++-реализации, структурно идентичные нашим). Без этого агенты
работали только через C++-подписку `ScActionInitiatedAgent` на
`action_initiated`, минуя онтологический уровень: ни один платформенный
механизм или scp-агент, ищущий `abstract_sc_agent`, не находил ни
одного из наших 26.

**Сделано:** добавлен `concepts/sc_agent_realizations.scs` — для
каждого из 26 агентов узел `sc_agent_of_<name>` с полным набором
обязательных связей (`nrel_sc_agent_key_sc_elements`,
`nrel_primary_initiation_condition`, `nrel_initiation_condition_and_result`,
`nrel_inclusion` → реализация `<- platform_dependent_abstract_sc_agent`
с `nrel_sc_agent_program`, указывающим на реальные `src/agent/*.{hpp,cpp}`).
Аргументы в шаблонах условия/результата привязаны позиционно через
`rrel_1..rrel_N` — ровно так, как `ScAction::SetArgument(size_t idx, ...)`
в реальности резолвит `idx` в `ScKeynodes::GetRrelIndex(idx)` (арность
`N` для каждого агента взята из его `GetArguments<N>()` в исходном коде,
не придумана). `nrel_sc_agent_action_class` сознательно не использована —
проверено по `section_subjdomain_of_sc_agents_content.scs`: в список
`rrel_explored_relation` (канонический/обязательный набор) она не входит,
а во всех реальных `lib_c_agents`-образцах класс действия просто
перечислен в `nrel_sc_agent_key_sc_elements` — этому и следует новый файл.

Это не правка ни одного из 26 агентов и не новая доменная семантика —
только формализация уже существующего поведения на онтологическом
уровне; выбор коллизий с существующими идентификаторами исключён
(проверено `grep` по всей KB перед добавлением). Дополняет, а не
заменяет `concepts/sc_agents.scs`: тот файл описывает 10-12 семейств
агентов на уровне процессов жизненного цикла
(`nrel_implements_process`), этот — 26 конкретных реализаций на уровне
платформенной регистрации.

Проверено: сборка KB через `sc-builder --clear` — чисто (34820
элементов, было 31841); `ctest` — 99 из 99, включая
`LoadRealKbTest.EveryProjectKeynodeResolvesAgainstExistingKbElements`
(§7.7, п.1) — подтверждает отсутствие коллизий с существующими
project keynodes.

Файлы: `ostis_kb_user_ecosystem/concepts/sc_agent_realizations.scs`.

### 4.5. Сквозная проверка ссылочной целостности онтологии (было: не упомянуто в исходном аудите — найдено 03.09.2026)

**Проблема была:** ни разу за время ведения `ostis_kb_user_ecosystem` не
проводилась механическая проверка того, что каждый идентификатор,
используемый как класс/тип, где-то реально объявлен. Без такой
проверки `sc-builder` тихо резолвит отсутствующий идентификатор по
схеме find-OR-CREATE — не выдавая ошибки, просто создавая
нетипизированный узел (тот же класс проблемы, что решает
`LoadRealKbTest` для C++ keynodes, только внутри самой онтологии).

**Сделано:** прогнан скрипт-аудит по всем 96 файлам `.scs` в трёх
разрезах — (1) каждый `nrel_*`/`rrel_*`, используемый где-либо, имеет
объявление `<- sc_node_(non_)role_relation`; (2) каждая из 94 секций в
дереве декомпозиции `main.scs` содержательно объявлена; (3) каждый
`concept_*`/`agent_*`/`action_*`/`rule_*`/`affirmation_*`/`definition_*`/
`axiom_*`/`scenario_*`/`example_*` объявлен хотя бы одной из двух форм
(subject-старт `X\n<- ...;` или target лейбловой дуги `-> X: (*...*);;`,
второй формой в KB типизируется большинство `definition_*`/`axiom_*`/
`affirmation_*`/`rule_*`). Найдено и исправлено:

- `concept_aggregation_rule` (`scenarios/scenario_explanation_construction.scs`) —
  нигде не объявленный класс; заменён на `concept_sc_rule`, базовый
  класс, которым типизируются все остальные правила проекта.
- `concept_trust_for_task` (`examples/example_trust.scs`) — нигде не
  объявленный класс, избыточно и ошибочно дополнительно типизировал
  `trust_level_energy`, уже корректно типизированный `concept_trust_level`
  двумя строками выше; строка удалена.
- `section_assignment_example` (`scenarios/scenario_medical_diagnostics.scs`,
  52 строки) — неподключённый нигде черновой фрагмент, дублирующий
  назначение `subtask_image_segmentation`, но другому исполнителю
  (`intelligent_system_image_analyzer`), не входящему в
  `community_medical_diagnostic` — прямо противоречил основному
  сценарию файла. Удалён целиком.
- `section_explanation_example` (`scenarios/scenario_explanation_construction.scs`,
  55 строк) — неподключённый нигде черновой дубликат «реифицированного
  акта построения объяснения» под другими именами. Удалён целиком.

Оба удалённых блока проверены на изоляцию: ни один из их
идентификаторов (`assignment_42`, `high_priority`,
`intelligent_system_image_analyzer`, `interaction_context_med_diagnostics`,
`explanation_77`, `integrated_result_19` и др.) нигде больше в KB не
встречается — удаление ничего не сломало.

Проверено: сборка KB через `sc-builder --clear` — чисто, 34579
элементов (34820 после §4.4, минус удалённые блоки); `ctest` — 99 из
99; повторный прогон всех трёх проверок — новых находок нет; продакшен
`repo/` не тронут (контрольная сумма идентична до и после).

Файлы: `ostis_kb_user_ecosystem/scenarios/scenario_explanation_construction.scs`,
`ostis_kb_user_ecosystem/scenarios/scenario_medical_diagnostics.scs`,
`ostis_kb_user_ecosystem/examples/example_trust.scs`.

## 5. Сделано частично

Пункты, где основная претензия исходного текста закрыта, но явно
названные в том же пункте хвосты остаются открытыми.

### 5.1. `AgentRegisterPartialResult` — акт исполнения, аудит, провенанс (было: п.6.2)

**Проблема была:** агент классифицировал результат и безусловно
устанавливал статус `completed`, не проверяя допуск и не создавая
`ExecutionAct`/`AuditRecord`/`ProvenanceRecord`.

**Сделано:** два гейта допуска (`system == nrel_assigned_to(subtask)`,
`nrel_subtask_status(subtask) == in_progress` — иначе `FinishWithError`);
минимальный набор из формулы аудита:
`ExecutionAct` (+ `nrel_executes`), `AuditRecord` (+ `nrel_recorded_in`),
по отдельной `ProvenanceRecord` для результата и для записи аудита
(`nrel_has_provenance` + `nrel_recorded_by_sc_agent`).

**Осталось:** версия плана, реальное время исполнения, проверка
целостности передачи результата — сам аудит выносил их за рамки
минимального набора ("как минимум").

Файлы: `src/agent/agent_register_partial_result.{hpp,cpp}`,
`tests/test_agent_register_partial_result.cpp` (4 теста),
`agents_vault/AgentRegisterPartialResult.md`.

### 5.2. `AgentResolveByTrustWeighted` — акт разрешения конфликта (было: п.6.4)

**Проблема была:** агент выбирал результат с максимальным доверием и сразу
помечал конфликт разрешённым через `IntegratedResult -[resolves_conflict]->
conflict`, без овеществлённого акта разрешения.

**Сделано:** создаётся `ConflictResolutionAct` (специализированный как
`concept_resolution_by_trust_weighting`), конфликт связывается с ним через
`nrel_resolved_by`, `nrel_conflict_status` переводится в `resolved`;
`resolves_conflict` теперь относится не к `IntegratedResult`, а к отдельно
порождаемому `Explanation` (`explanation -[nrel_resolves_conflict]->
conflict`, `IntegratedResult -[nrel_explained_by]-> explanation`) — снято
расхождение, ранее отмеченное в `relations/relations_trust.scs`.

**Осталось:** верификация кандидатов; учёт качества, контекста, ограничений
и требуемого порога помимо доверия; обработка равенства оценок и
недостаточного доверия; эскалация либо отказ при неразрешимости;
содержательный текст объяснения (узел `Explanation` создаётся пустым).
Выбор по доверию остаётся единственной реализованной стратегией разрешения
конфликта, а не одной из нескольких.

Файлы: `src/agent/agent_resolve_by_trust_weighted.{hpp,cpp}`,
`tests/test_agent_resolve_by_trust_weighted.cpp` (3 теста),
`agents_vault/AgentResolveByTrustWeighted.md`.

### 5.3. `AgentComputePiMetric` — реакция на `π < 1` (было: п.6.7)

**Проблема была:** агент корректно вычислял `π`, но не реализовывал
поведение правила при `π < 1`: не создавал нарушение, не запускал
восполнение провенанса.

**Сделано:** при `π < 1` на непустой группе создаётся
`concept_existence_constraint_violation` (`concepts/integrity.scs`) —
`transparency_gap` в KB нет как формального класса, только как
неструктурированный текст правила, поэтому переиспользован ближайший
существующий подкласс; нарушение связано с проверяемой `elements_group`
через `nrel_concerns` (тот же приём, что и в §5.4).

**Осталось:** восстановление пропущенного провенанса
(`rule_fill_missing_provenance`) и разделение ответственности на
`AgentProvenanceCompletenessValidator`/`AgentProvenanceRegistrar`/
`AgentProvenanceRecovery` — не реализовано, это новые агенты (§7.2).

Файлы: `src/agent/agent_compute_pi_metric.{hpp,cpp}`,
`tests/test_agent_compute_pi_metric.cpp` (4 теста),
`agents_vault/AgentComputePiMetric.md`.

### 5.4. `AgentValidateCardinalityConstraint` — типизированная связь нарушения (было: п.6.8)

**Проблема была:** агент проверяет только кардинальные ограничения и при
создании нарушения не связывает его формальным отношением с проверяемой
сущностью.

**Сделано:** добавлена `nrel_concerns` (`IntegrityViolation → sc_node`,
`concepts/integrity.scs`) — домен второй позиции общий (`sc_node`), так как
своего надкласса "любая сущность" в онтологии не формализовано; нарушение
теперь связано с `target` через `nrel_concerns`.

**Осталось:** `violatesConstraint` и `detectedIn` из предпочтительной схемы
аудита — нужны новые аргументы действия, которых сейчас нет ни у агента, ни
у вызовов в `rules_integrity_validation.scs`. Более широкая претензия
пункта (агент покрывает только кардинальность, а не типы/диапазоны
отношений, ацикличность, допустимость назначения, полноту контекста/
провенанса/аудита/объяснения, конфликт, безопасность, переходы жизненного
цикла) не снята вовсе — расширение охвата требует инфраструктуры §7.1/§7.2.

Файлы: `src/agent/agent_validate_cardinality_constraint.{hpp,cpp}`,
`tests/test_agent_validate_cardinality_constraint.cpp` (3 теста),
`agents_vault/AgentValidateCardinalityConstraint.md`.

### 5.5. `AgentDetectAnomalousRequestRate` — классификация инцидента (было: п.6.9)

**Проблема была:** агент создавал общий `concept_security_incident` без
выбора конкретного STRIDE-подкласса и без последующей управляемой реакции.

**Сделано:** инцидент дополнительно классифицируется как
`concept_incident_denial_of_service` — сигнал агента чисто объёмный
(`rate`/`baseline`), содержимого запросов он не видит, значит обосновать
может только DoS, не Tampering; тем самым для этого конкретного агента
дизъюнкция "DoS либо Tampering" из правила разрешается однозначно.

**Осталось:** связь инцидента с субъектом/системой/запросом/периодом
наблюдения, запись аудита, снижение `t_sec`, приостановка маршрута, запуск
проверки доступа/переназначения/эскалации. Проверено по гл. 2 диссертации:
детектирование аномалий/STRIDE там не формализовано вовсе — есть только
`ν_sec ∈ {0,1}` ("отсутствие инцидента безопасности") как вход EMA-
обновления `t_sec`, а эта формула целиком принадлежит
`AgentUpdateTrustSecurity`; понижать `t_sec` из детектора значило бы
дублировать чужую формулу. Для остальных пунктов ни правило, ни аудит, ни
диссертация не называют конкретных отношений (в отличие, например, от
`isInputFor` в §4.3) — закрыть их значит придумывать новую онтологию, а не
читать существующую.

Файлы: `src/agent/agent_detect_anomalous_request_rate.{hpp,cpp}`,
`tests/test_agent_detect_anomalous_request_rate.cpp` (3 теста),
`agents_vault/AgentDetectAnomalousRequestRate.md`.

### 5.6. `AgentBuildExplanation` — структурная реификация объяснения (было: строка `AgentBuildExplanation` в §7.5 — новый агент, а не правка существующего)

**Проблема была:** ни один агент не заполнял
`concept_explanation_construction_act` по шаблону `prov_template_explanation`
(`axioms/axioms_provenance.scs`) — `AgentResolveByTrustWeighted` создавал
лишь голый узел `concept_explanation` как структурную заглушку, явно
помечая содержательное наполнение как нереализованное.

**Сделано:** один агент на все четыре правила `rule_build_local_explanation`/
`_global`/`_contrastive`/`_counterfactual` (`rules/rules_explanation.scs`),
тем же приёмом, что `AgentComputePiMetric` для π_proc/π_dec/π_data —
конкретный класс объяснения выбирает вызывающая сторона аргументом. Агент
создаёт узел объяснения нужного класса, связывает его с explanandum через
`nrel_supported_by_explanation` (и дополнительно `nrel_explained_by`, если
explanandum уже классифицирован `concept_integrated_result` — воспроизводит
`scenario_explanation_construction.scs`), и порождает овеществлённый
`concept_explanation_construction_act` со всеми пятью ролями шаблона
(`rrel_explanandum`, `rrel_explanation_type`, `rrel_uses_results`,
`rrel_applies_rules`, `rrel_based_on`, `rrel_construction_time`) —
последние четыре только если вызывающая сторона передала соответствующий
аргумент.

**Осталось:** содержательный текст объяснения (`nrel_main_idtf` с описанием);
специфичный для типа контент — агрегация локальных объяснений в глобальное,
поиск различий для контрастивного, минимальное изменение входа для
контрфактического — ни для одного из них в тексте правил нет алгоритма,
агент лишь реифицирует то, что уже подготовил вызывающий код; связывание
использованных правил/источников через `nrel_prov_used`
(`concepts/provenance.scs`) — его домен `concept_prov_activity` →
`concept_prov_entity` не установлен ни для узла объяснения, ни для узлов
правил/источников, типизировать их так здесь означало бы вводить новую
онтологию, а не переиспользовать существующую (тот же класс решения, что
заблокировал §7.4); проверка "объяснение уже существует" из предусловия
правила (`¬supported_by_explanation(r,_)`) — агент не проверяет и создаст
второе объяснение при повторном вызове; сам агент пока не вызывается ни из
`AgentResolveByTrustWeighted`, ни откуда-либо ещё.

Файлы: `src/agent/agent_build_explanation.{hpp,cpp}`,
`src/keynodes/explanation_keynodes.hpp`,
`tests/test_agent_build_explanation.cpp` (4 теста),
`agents_vault/AgentBuildExplanation.md`.

### 5.7. `AgentValidateIntegration` — валидация перед публикацией (было: строка `AgentVerifyResult` в §7.5 — новый агент, покрывающий часть заявленной области)

**Проблема была:** `rule_validate_integration` (`rules/rules_integration.scs`)
требует перед фиксацией `ir` проверить `∃e. explained_by(ir,e) ∧
¬∃c. conflicts_with(ir,c)`, но до §5.6 `nrel_explained_by` не создавался
ни одним универсальным агентом (только заглушкой внутри
`AgentResolveByTrustWeighted`) — проверка первого условия была бы
проверкой того, что никто не производит.

**Сделано:** агент проверяет оба условия правила и пишет булев индикатор
(`1.0`/`0.0`) в `value_link` — тот же приём, что у
`AgentDispatchReadySubtask`/`AgentDetectAnomalousRequestRate`. Условие
конфликта проверяется в обоих направлениях дуги
(`nrel_conflicts_with` объявлено симметричным, но в сценариях KB
материализовано непоследовательно — `scenario_collective_decision.scs`
дублирует дугу в обе стороны, `scenario_smart_city_traffic.scs` — нет;
проверка только исходящего направления пропустила бы второй случай).

**Осталось:** "иначе вернуть управление `rule_resolve_by_*`" — агент не
инициирует ничего при невалидности, передача управления остаётся задачей
внешнего оркестратора, которого нет; остальная заявленная область
`AgentVerifyResult` ("проверяет ограничения, объяснение, провенанс и
конфликты") шире этого агента — ограничения покрывает
`AgentValidateCardinalityConstraint` (§5.4), провенанс —
`AgentComputePiMetric` (§5.3), оба независимо; агент не проверяет, что
`integrated_result` действительно классифицирован
`concept_integrated_result`; устаревшая (но не стёртая) дуга
`nrel_conflicts_with` от уже разрешённого конфликта по-прежнему
трактуется как активный конфликт — стирание таких дуг никем не
реализовано.

Файлы: `src/agent/agent_validate_integration.{hpp,cpp}`,
`src/keynodes/integration_keynodes.hpp` (расширен: `nrel_conflicts_with`,
`action_validate_integration`), `tests/test_agent_validate_integration.cpp`
(4 теста), `agents_vault/AgentValidateIntegration.md`.

### 5.8. `AgentValidateProvenanceChain` — достижимость запроса по цепочке провенанса (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_integrity_validation.scs`, 7-е из 7 правил, ранее помеченное "не реализовано — a fundamentally different task")

**Проблема была:** `rule_verify_transitive_closure`
(`rules/rules_provenance_completion.scs`) и дублирующее его
`rule_validate_provenance_chain` (`rules/rules_integrity_validation.scs`)
требуют транзитивной достижимости `concept_user_request` по
`nrel_derived_from` от `ir` — единственная в проекте задача обхода графа,
а не подсчёта/проверки одной дуги; `Implementation Backlog.md` прямо
называл её "a fundamentally different task."

**Сделано:** один агент на оба правила (тот же приём, что у
`AgentValidateIntegration` для их собственной пары дублирующихся правил).
Обход в ширину по `nrel_derived_from` с множеством посещённых узлов —
защита от зацикливания, поскольку отношение объявлено транзитивным, но не
антирефлексивным. Результат — булев индикатор; при недостижимости —
`concept_existence_constraint_violation`, связанное с `ir` через
`nrel_concerns` (тот же приём, что у `AgentComputePiMetric`/
`AgentValidateCardinalityConstraint`). Тестами подтверждена корректная
терминация на графе с циклом. Закрывает 7-е из 7 правил
`rules_integrity_validation.scs` — файл теперь покрыт полностью.

**Осталось:** снижение π_proc/π_data и запуск `rule_fill_missing_provenance`
— вне области агента (нет входного metric_level, эвристика без
алгоритма); эскалация `agent_provenance_registrar` не реализована (нет
получателя); **сама проверяемая связь `nrel_derived_from` пока не
создаётся ни одним агентом проекта** — используется только в прозе
`axioms/axioms_provenance.scs`, ни в одном сценарии; наблюдение по
онтологии, не исправление — `nrel_derived_from` объявлено доменом
`concept_processing_result → concept_processing_result`
(`relations/relations_provenance.scs`), а `concept_user_request` не
является `concept_processing_result` (`concepts/interactions.scs`) —
дуга, которую требует правило, формально выходит за объявленный домен
отношения; агент только читает существующие дуги и не создаёт новых,
поэтому это оставлено как открытое наблюдение, а не решено здесь.

Файлы: `src/agent/agent_validate_provenance_chain.{hpp,cpp}`,
`src/keynodes/provenance_keynodes.hpp` (расширен: `nrel_derived_from`,
`concept_user_request`, `action_validate_provenance_chain`),
`tests/test_agent_validate_provenance_chain.cpp` (5 тестов),
`agents_vault/AgentValidateProvenanceChain.md`.

### 5.9. `AgentExcludeFromCoalition` — исключение отказавшей системы (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_coalition_reconfiguration.scs`, ранее "not implemented")

**Проблема была:** ни один агент не реагировал на отказ подзадачи —
исполнитель оставался числиться членом коалиции, даже когда назначенная
ему подзадача перешла в статус `failed`.

**Сделано:** механическая часть `rule_on_system_failure`. Все три условия
правила проверяются буквально (`subtask_status = failed`, наличие
`assigned_to`, конкретная связь `member_of(s, K)` с ПЕРЕДАННОЙ вызывающей
стороной коалицией K, а не любой коалицией исполнителя) — при выполнении
всех трёх дуга `nrel_member_of` стирается, индикатор = 1.0; иначе — без
изменений, индикатор = 0.0 (невыполненное условие, не ошибка).

**Осталось:** запуск `rule_coalition_optimization` для подзадач, ранее
назначенных исключённой системе — комбинаторная задача распределения без
алгоритма в тексте правил (`rules_allocation.scs`, тот же класс блокировки,
что у §7.3/§7.4); сама переданная `community` не выводится агентом
самостоятельно — вызывающая сторона обязана знать, из какой именно
коалиции исключать.

Файлы: `src/agent/agent_exclude_from_coalition.{hpp,cpp}`,
`src/keynodes/coalition_keynodes.hpp` (новый),
`tests/test_agent_exclude_from_coalition.cpp` (4 теста),
`agents_vault/AgentExcludeFromCoalition.md`.

### 5.10. `AgentReassignOnResourceExhaustion` — массовое переназначение при истощении ресурса (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_coalition_reconfiguration.scs`, ранее "not implemented")

**Проблема была:** истощение ресурса `L_s` системы не вело ни к какому
изменению статусов подзадач, которые эта система всё ещё числилась
исполняющей.

**Сделано:** механическая часть `rule_on_resource_exhaustion`. Агент сам
находит все подзадачи, назначенные переданной системе (обратный поиск по
`nrel_assigned_to`, без группы-аргумента от вызывающей стороны), и
переводит статус каждой в `concept_subtask_status_reassigned`
(функциональное отношение — старая дуга стирается). Количество
переназначенных подзадач записывается числом в индикатор; ноль подзадач —
`0.0`, не ошибка.

**Осталось:** сам факт `resource_exhausted` не проверяется и не
вычисляется агентом — принимается как заданный вход; запуск
`rule_coalition_optimization` не реализован (та же причина, что у §5.9);
переназначенные подзадачи не получают нового исполнителя —
`agent_task_allocator` не реализован.

Файлы: `src/agent/agent_reassign_on_resource_exhaustion.{hpp,cpp}`,
`src/keynodes/coalition_keynodes.hpp` (общий с §5.9),
`tests/test_agent_reassign_on_resource_exhaustion.cpp` (4 теста),
`agents_vault/AgentReassignOnResourceExhaustion.md`.

### 5.11. `AgentCheckAcyclicity` — проверка ацикличности графа зависимостей подзадач (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_decomposition.scs`, ранее "not implemented")

**Проблема была:** `definition_dependency_graph` (`concepts/tasks.scs`)
формально требует, чтобы граф `nrel_subtask_depends_on` над множеством
подзадач декомпозиции T(q) был ацикличным, но ни один агент это не
проверял — цикл в данных мог образоваться (например, из-за ошибки
шаблона декомпозиции) и остаться незамеченным.

**Сделано:** механическая часть `rule_check_acyclicity`. Агент собирает
T(q) через прямой обход `nrel_decomposed_into(request, ·)`, затем
выполняет DFS по `nrel_subtask_depends_on` с раздельными множествами
`onStack` (текущий путь обхода) и `visited` (все посещённые узлы) —
цикл фиксируется только при переходе в узел из `onStack`, а не при любом
повторном посещении, что отличает настоящий цикл от сходящихся
(ромбовидных) путей корректного DAG. При обнаружении цикла создаётся
новый класс `concept_acyclicity_constraint_violation`
(`concepts/integrity.scs`, подкласс `concept_integrity_violation`,
формализован по тому же образцу, что и
`concept_cardinality_constraint_violation`/`concept_existence_constraint_violation`
у §5.4), связанный с `request` через `nrel_concerns`; индикатор — `1.0`/`0.0`
в `value_link`, тем же приёмом, что у §5.8.

**Осталось:** «инициировать пересмотр шаблона декомпозиции» не
реализовано — `agent_task_decomposer` сам не реализован
(`rule_apply_decomposition_pattern`/`rule_recursive_decomposition` —
PARTIAL, требуют `sim_T` и библиотеку шаблонов, не определённых в
тексте, тот же класс блокировки, что у §7.3/§7.4/§5.9/§5.10); обход
проверяет ацикличность подграфа, ДОСТИЖИМОГО из T(request), а не только
рёбер строго между элементами T(request) — по всем сценариям проекта
разницы на практике нет, но формально это наблюдение, а не
исправленный дефект.

Файлы: `src/agent/agent_check_acyclicity.{hpp,cpp}`,
`src/keynodes/task_keynodes.hpp` (`nrel_decomposed_into`, `action_check_acyclicity` —
новые), `src/keynodes/integrity_keynodes.hpp`
(`concept_acyclicity_constraint_violation` — новый),
`concepts/integrity.scs`, `rules/rules_decomposition.scs`,
`tests/test_agent_check_acyclicity.cpp` (5 тестов),
`agents_vault/AgentCheckAcyclicity.md`.

### 5.12. `AgentValidateRightToExplanation` — гейт-проверка права на объяснение (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_security_privacy.scs`, ранее "READY", не реализовано)

**Проблема была:** `concept_property_right_to_explanation`
(`concepts/security_privacy.scs`) и `axiom_decision_requires_explanation`
(`axioms/axioms_security.scs`) формализуют право пользователя на
объяснение автоматизированного решения, но ни один агент это не
проверял — интегрированный результат мог быть выдан пользователю без
единой связанной объяснительной записи, причём именно для тех типов
запроса (поддержка решения, рекомендация), где это критично.

**Сделано:** условная гейт-проверка `rule_right_to_explanation`. Гейт
срабатывает при выполнении ОБОИХ условий: наличие хотя бы одной дуги
`ir -[delivered_to]-> ·` (конкретный получатель не проверяется — в
консеквенте правила он не упоминается) И классификация `request`
классом `concept_request_decision_support` либо
`concept_request_recommendation`. Если гейт сработал — валидность
определяется наличием `ir -[explained_by]-> ·` (то же отношение, что
проверяет `AgentValidateIntegration` и создаёт `AgentBuildExplanation`);
если не сработал — проверка вырожденно проходит независимо от
`explained_by`. При невалидном исходе узел нарушения классифицируется
СРАЗУ двумя классами — `concept_integrity_violation` и уже существующим
`concept_property_right_to_explanation` — по буквальному образцу
соседнего правила того же файла, `rule_enforce_consent_check`
("создать concept_integrity_violation типа
concept_property_consent_compliance"); новый подкласс онтологии не
потребовался.

**Осталось:** "agent_explanation_builder обязан построить" не
реализовано — агент не вызывает `AgentBuildExplanation` при обнаруженном
нарушении, только сигнализирует его отсутствие; "ir не может быть выдан
пользователю" не имеет точки принудительного применения — `AgentPublishResult`
не реализован (§7.5); связь `request -> integrated_result` не выводится
агентом самостоятельно — вызывающая сторона обязана знать, какой именно
запрос породил проверяемый результат (формального однохопового пути в
онтологии нет), тот же класс ограничения, что у `community` в §5.9.
Отдельное терминологическое наблюдение (не исправляется здесь): текст
ЭТОГО правила требует `explained_by` (`nrel_explained_by`), тогда как
соседняя аксиома `axiom_decision_requires_explanation` для похожего по
смыслу требования использует другое отношение — `supported_by_explanation`
(`nrel_supported_by_explanation`); реализовано буквально по тексту
правила.

Файлы: `src/agent/agent_validate_right_to_explanation.{hpp,cpp}`,
`src/keynodes/security_keynodes.hpp` (`nrel_delivered_to`,
`concept_request_decision_support`, `concept_request_recommendation`,
`concept_integrity_violation`, `concept_property_right_to_explanation`,
`action_validate_right_to_explanation` — новые), `rules/rules_security_privacy.scs`,
`tests/test_agent_validate_right_to_explanation.cpp` (5 тестов),
`agents_vault/AgentValidateRightToExplanation.md`.

### 5.13. `AgentEvaluateProgressiveDisclosure` — решение о прогрессивном раскрытии (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_user_adaptation.scs`, ранее "READY", не реализовано)

**Проблема была:** `concept_strategy_progressive_disclosure`
(`concepts/adaptive_interface.scs`) именует стратегию, но двухпороговое
условие её срабатывания ("глубина provenance-цепочки > 4 шагов ИЛИ
количество частичных результатов > 5") нигде не вычислялось.

**Сделано:** вычислимая часть `rule_progressive_disclosure`. Глубина —
длина самой длинной цепочки исходящих дуг `nrel_derived_from` от
переданного узла, вычисляется мемоизированным DFS с защитой от цикла
(`onStack`, тот же приём, что у §5.11); количество — число прямых детей
(`ConstPermPosArc`) переданной группы `partial_results_group`, тот же
приём `elements_group`, что у `AgentComputePiMetric` (§5.3). Индикатор
записывает САМО РЕШЕНИЕ правила (1.0 = раскрытие требуется), а не
«валидно/невалидно» — единственный агент модуля с такой полярностью,
явно задокументированной, чтобы не создавать ложной аналогии с
семейством `AgentValidate*`.

**Побочная находка:** соседнее правило `rule_choose_modality` того же
файла, ранее помеченное READY в
`agents_vault/Implementation Backlog.md` ("inputs are Boolean/class
flags already existing as sc-facts"), при ближайшем рассмотрении
таковым не оказалось: ни «ограничение визуального восприятия»
(`rrel_user_situation_context`), ни «preference_visual»
(`concept_user_preference`) не имеют типизированного представления —
`concept_user_preference` (`concepts/users.scs`) — плоский класс без
подразделения, единственный релевантный сценарий
(`scenario_intelligent_tutoring.scs`, `preference_ivan_visual`) несёт
эту информацию только в свободнотекстовом `nrel_main_idtf`. Реализация
потребовала бы объявления новой типизации онтологии, а не
операционализации существующей — оценка готовности исправлена в
Implementation Backlog.md, правило НЕ реализовано в рамках этой работы.

**Осталось:** «адаптивный интерфейс представляет верхний уровень» не
материализуется — ни один агент проекта не создаёт экземпляр
`concept_adaptive_interface`, `nrel_applies_strategy` не пишется; тот же
класс ограничения, что у §5.9/§5.10/§5.12 (нет получателя/потребителя
ниже по потоку). `rule_choose_modality` остаётся нереализованным (см.
выше).

Файлы: `src/agent/agent_evaluate_progressive_disclosure.{hpp,cpp}`,
`src/keynodes/adaptive_interface_keynodes.hpp` (новый файл, первый агент
домена), `rules/rules_user_adaptation.scs`,
`tests/test_agent_evaluate_progressive_disclosure.cpp` (5 тестов),
`agents_vault/AgentEvaluateProgressiveDisclosure.md`.

### 5.14. `AgentChooseModality` — выбор модальности взаимодействия (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_user_adaptation.scs`, ранее "READY", оценка исправлена на PARTIAL в §5.13, доведено до реализации)

**Проблема была:** `concept_interface_modality` и все шесть её
подклассов (`concepts/adaptive_interface.scs`) были полностью
типизированы, но правило, выбирающее между ними, не имело
типизированных входов: ни «ограничение визуального восприятия»
(`rrel_user_situation_context`), ни «preference_visual»
(`concept_user_preference`) не существовали иначе как в прозе правила
(находка при работе над §5.13).

**Сделано:** точечное расширение онтологии двумя узкими подклассами,
называющими ровно то, что уже называла проза правила, а не новую
семантику: `concept_visual_perception_constraint` (`<= nrel_inclusion:
concept_user_situation_context`, `concepts/contexts.scs`) и
`concept_preference_visual_modality` (`<= nrel_inclusion:
concept_user_preference`, `concepts/users.scs`). Существующий сценарный
фикстур `scenario_intelligent_tutoring.scs` (`preference_ivan_visual`,
прежде описанный только свободным текстом) дополнен типизированной
классификацией. Агент реализует трёхветочный выбор в порядке,
буквально заданном правилом (If/Elif/Default): ограничение восприятия
→ `concept_modality_voice`; иначе предпочтение → `concept_modality_visual`;
иначе → `concept_modality_text`; результат — сама выбранная
разделяемая модальность-класс, а не числовой индикатор (тот же приём
нечислового результата, что у `AgentBuildExplanation`, §5.6).

**Осталось:** `nrel_uses_modality(interface, modality)` не пишется —
ни один агент проекта не создаёт экземпляр `concept_adaptive_interface`
(тот же класс ограничения, что у §5.13); прочие ситуационные
индикаторы, упомянутые в прозе соседних правил того же файла
(mobile/on-the-go — `rule_choose_presentation_template`; night-shift/
low-light — `rule_context_aware_layout`), и прочие виды пользовательских
предпочтений остаются неформализованными — намеренно вне рамок этой
правки.

Файлы: `src/agent/agent_choose_modality.{hpp,cpp}`,
`src/keynodes/adaptive_interface_keynodes.hpp` (расширен),
`concepts/contexts.scs`, `concepts/users.scs`, `rules/rules_user_adaptation.scs`,
`scenarios/scenario_intelligent_tutoring.scs` (типизация существующего
фикстура), `tests/test_agent_choose_modality.cpp` (5 тестов),
`agents_vault/AgentChooseModality.md`.

### 5.15. `AgentRecordProvEntity` — регистрация PROV Entity (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_provenance_completion.scs`, ранее "READY", не реализовано)

**Проблема была:** `concepts/provenance.scs` полностью формализует
PROV-O триаду (`concept_prov_entity`/`_activity`/`_agent` и их
отношения, согласовано с W3C PROV-O), но ни один агент её не заполнял —
вся типизация существовала невостребованной.

**Сделано:** для результата обработки `result` создаётся
`concept_prov_entity`, связывается с производящей деятельностью через
`nrel_prov_was_generated_by`, с производящим агентом — через
`nrel_prov_was_attributed_to`, и устанавливается как цель
функционального `nrel_has_provenance` на `result`
(`SetFunctionalRelationTarget`, а не `LinkByRelation`, чтобы корректно
заменить существующую цель, если таковая есть).

**Побочная находка:** `AgentRegisterPartialResult`
(`rules/rules_cooperation.scs`) уже создаёт для каждого частичного
результата собственную провенансную запись — обобщённый узел,
классифицированный `concept_provenance_record` БЕЗ PROV-O
специализации, тоже через `nrel_has_provenance`. Поскольку отношение
функционально, вызов `AgentRecordProvEntity` ПОСЛЕ
`AgentRegisterPartialResult` заменит обобщённую запись типизированной
PROV-сущностью. Текст модели не определяет, какой из двух механизмов
канонический — зафиксировано как открытый вопрос, не решается в рамках
этой правки.

**Осталось:** «When: new concept_processing_result(r) появляется в
sc-памяти» не реализовано как автоматический триггер по событию — ни
один агент проекта не подписан на такие события; вызывающая сторона
инициирует регистрацию явно, как и везде в модуле. `activity`/`agent`
должны существовать заранее (обычно `activity` — через
`AgentRecordProvActivity`, §5.16), сам агент их не создаёт.

Файлы: `src/agent/agent_record_prov_entity.{hpp,cpp}`,
`src/keynodes/provenance_keynodes.hpp` (расширен полной лексикой
PROV-O), `rules/rules_provenance_completion.scs`,
`tests/test_agent_record_prov_entity.cpp` (3 теста),
`agents_vault/AgentRecordProvEntity.md`.

### 5.16. `AgentRecordProvActivity` — регистрация PROV Activity (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_provenance_completion.scs`, ранее "READY", не реализовано)

**Проблема была:** та же, что у §5.15, для `concept_prov_activity` и
её отношений (`nrel_prov_used`, `nrel_prov_was_associated_with`).

**Сделано:** создаётся `concept_prov_activity` с ролями `rrel_actor`
(цель `actor`) и `rrel_input` (цель `input`), связывается с `input`
через `nrel_prov_used` и с `prov_agent` через
`nrel_prov_was_associated_with`; необязательный `timestamp` (последний
аргумент, тот же приём, что `rrel_construction_time` у
`AgentBuildExplanation`, §5.6) привязывается ролью `rrel_timestamp`,
если передан.

**Осталось:** функция `agent_of(s_a)` (отображение исполняющего
sc-агента на его PROV-агента) нигде не формализована формулой в тексте
— `actor` и `prov_agent` передаются раздельными аргументами вместо
вычисления одного из другого. `rrel_actor` формально объявлен как
`concept_interaction_act -> concept_actor`
(`relations/role_relations.scs`), тогда как овеществлённая деятельность
здесь — `concept_prov_activity`, не обязательно `concept_interaction_act`
— наблюдение того же класса, что и у `nrel_derived_from` в §5.8, не
исправляется здесь. Автоматический триггер по событию исполнения
процесса не реализован — та же причина, что у §5.15.

Файлы: `src/agent/agent_record_prov_activity.{hpp,cpp}`,
`src/keynodes/provenance_keynodes.hpp` (общий с §5.15),
`tests/test_agent_record_prov_activity.cpp` (3 теста),
`agents_vault/AgentRecordProvActivity.md`.

### 5.17. `AgentHandleNoAdmissibleAssignment` — обработка отсутствия допустимого назначения (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_allocation.scs`, ранее "READY", не реализовано)

**Проблема была:** когда ни одна ИС не является допустимым исполнителем
подзадачи, правило предлагает два пути (рекурсивная декомпозиция либо
нарушение целостности), но ничто не решало, какой путь применим, и не
создавало само нарушение.

**Сделано:** механическая часть правила. Посылка ("нет допустимого
исполнителя") принимается уже установленной вызывающей стороной — сама
`match(σ,s)` определена лишь частично (только `trust(s,τ,h)` из шести
слагаемых вычислим, см. §7.4/Implementation Backlog.md), тот же приём,
что у §5.9/§5.10 в отношении `failed`/`resource_exhausted`. Ветвление
консеквента разрешается через `affirmation_subtask_executability`
(`concepts/tasks.scs`: всякая подзадача либо исполнима, либо
декомпозируема) — если σ классифицирована `concept_subtask_composite`,
путь декомпозиции применим (индикатор 1.0, нарушение не создаётся);
иначе — создаётся `concept_integrity_violation`, связанный с σ через
`nrel_concerns` (индикатор 0.0).

**Осталось:** сама рекурсивная декомпозиция
(`rule_apply_decomposition_pattern`) не выполняется — она сама не
реализована (требует `sim_T` и библиотеку шаблонов, не определённых в
тексте); «вернуть управление `agent_task_decomposer`» не реализовано —
нет оркестратора жизненного цикла (§7.1); только `IsElement` на
аргументах.

По итогам этого обзора `rule_handle_no_admissible_assignment` было
последним действительно READY и нереализованным правилом во всём
`agents_vault/Implementation Backlog.md` — всё оставшееся там теперь
PARTIAL (упирается в неопределённую формулу/таблицу, например
`cognitive_load(u)`, недостающие пять слагаемых `match()`, таблицу
терминологии) либо NO (интерпретация запроса, выбор шаблона
декомпозиции — LLM-задачи, а не формулы).

Файлы: `src/agent/agent_handle_no_admissible_assignment.{hpp,cpp}`,
`src/keynodes/allocation_keynodes.hpp` (новый файл, первый агент
домена), `rules/rules_allocation.scs`,
`tests/test_agent_handle_no_admissible_assignment.cpp` (3 теста),
`agents_vault/AgentHandleNoAdmissibleAssignment.md`.

### 5.18. `AgentEvaluateContextAwareLayout` — детектирование условия контекстно-зависимой компоновки (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_user_adaptation.scs`, ранее PARTIAL, гейт закрыт по образцу §5.14)

**Проблема была:** `concept_strategy_context_aware_layout`
(`concepts/adaptive_interface.scs`) называла стратегию с самого начала
этого прохода реализации, но ни одно из двух условий её срабатывания
(«night-shift», «low-light») не имело типизированного представления —
тот же класс пробела, что нашёлся и закрылся для `rule_choose_modality`
(§5.14).

**Сделано:** добавлены два узких подкласса-флага
`concept_night_shift_context` и `concept_low_light_context`
(`<= nrel_inclusion: concept_user_situation_context`,
`concepts/contexts.scs`) — два отдельных, а не один общий, поскольку
правило называет их как два самостоятельных условия. Агент проверяет
классификацию `situation` любым из двух флагов (ИЛИ); при срабатывании
присоединяет уже существующий `concept_strategy_context_aware_layout`
как дочерний узел `result_level` — единый маркер на весь пакет
последствий, поскольку ни у «тёмной темы», ни у «сокращения элементов»
нет своего типизированного представления, а для «шрифт ×1.10» нет
представления текущего размера шрифта, которое можно было бы умножить.
Индикатор — decision-полярность (1.0 = сработало), тот же приём, что у
§5.13.

**Осталось:** «снизить количество одновременно показываемых элементов»
не квантифицировано нигде в тексте — тот же класс блокировки, что у
`cognitive_load(u)` в `rule_choose_presentation_template`; формула
шрифта (×1.10) сама по себе не содержит неопределённости, но применять
её не к чему без представления текущего размера шрифта в KB;
`nrel_applies_strategy` не пишется — нет экземпляра
`concept_adaptive_interface`, та же причина, что у §5.13/§5.14.

Рассмотрены и отклонены как недостаточно узкие: `rule_enforce_consent_check`
(нужны сразу новое отношение «покрывает согласием» И механизм
валидности/истечения — не одна типизация); `rule_on_constraint_violation`
(«понизить доверие» без формулы величины понижения — придумывание
величины было бы содержательным решением модели, а не исправлением
типизации, вне рамок); `rule_audit_log_integrity` (инвариант для
принудительного соблюдения, а не вычисляемое значение — не укладывается
в реактивную модель агента вообще).

Файлы: `src/agent/agent_evaluate_context_aware_layout.{hpp,cpp}`,
`src/keynodes/adaptive_interface_keynodes.hpp` (расширен),
`concepts/contexts.scs`, `rules/rules_user_adaptation.scs`,
`tests/test_agent_evaluate_context_aware_layout.cpp` (4 теста),
`agents_vault/AgentEvaluateContextAwareLayout.md`.

### 5.19. `AgentLinkExplanationToProvenance` — связывание акта построения объяснения с провенансом (было: не упомянуто в §6/§7 этого документа — найдено через `agents_vault/Implementation Backlog.md`, `rules_provenance_completion.scs`, ранее "READY на бумаге, PARTIAL на практике", блокер снят §5.15/§5.16)

**Проблема была:** `rule_link_explanation_to_provenance` требует трёх
PROV-O фактов для каждого `explanation_construction_act`:
`wasDerivedFrom(e, r_i)` для каждого использованного результата,
`PROV:used(activity_of_e, rule_i)` для каждого применённого правила,
`wasAttributedTo(e, agent_explanation_builder)` — один раз. Все три
отношения (`nrel_prov_was_derived_from`/`nrel_prov_used`/
`nrel_prov_was_attributed_to`, `concepts/provenance.scs`) и обе роли
(`rrel_uses_results`/`rrel_applies_rules`, `concepts/explanations.scs`)
существовали с самого начала — правило было заблокировано только тем,
что ни один агент не создавал сами PROV-O узлы (entity/activity/agent)
для явления `e`. Этот блокер снят §5.15/§5.16 (`AgentRecordProvEntity`/
`AgentRecordProvActivity`) днём ранее.

**Сделано:** добавлен `AgentLinkExplanationToProvenance` — читает
`rrel_uses_results`/`rrel_applies_rules` с уже овеществлённого
`construction_act` (порождается §5.6 `AgentBuildExplanation`) и
связывает их напрямую (без промежуточной типизации как
`concept_prov_entity`) с переданными вызывающей стороной
`explanation_entity`/`explanation_activity`/`builder_prov_agent` —
готовые PROV-O узлы аргументами, тот же приём, что и `agent_of()` в
§5.16. Единственное новое в C++: `ForEachRoleTarget`
(`common/relation_io.hpp`) — обход *всех* целей роли из одного
источника, читающий-симметричный аналог существовавшего `LinkByRole`.
В отличие от §5.14/§5.18, никакой новой онтологии не потребовалось —
чистое монтирование уже существующих отношений.

**Осталось:** `r_i`/`rule_i` не типизируются этим агентом как
`concept_prov_entity` (формальный домен обеих связей) — текст правила
называет целью именно их, не производную PROV-сущность, поэтому
решено связать их как есть, той же логикой "доверять вызывающей
стороне в вопросах типизации", что и весь остальной модуль, а не
изобретать дополнительный уровень обёрток; `agent_of(agent_explanation_builder)`
не вычисляется — готовый `concept_prov_agent` передаётся аргументом.
`rules_provenance_completion.scs` теперь 5 из 6 правил — остался
только `rule_fill_missing_provenance` (эвристика без алгоритма в
тексте).

Файлы: `src/agent/agent_link_explanation_to_provenance.{hpp,cpp}`,
`src/common/relation_io.hpp` (расширен — `ForEachRoleTarget`),
`src/keynodes/provenance_keynodes.hpp` (расширен —
`nrel_prov_was_derived_from`, `action_link_explanation_to_provenance`),
`rules/rules_provenance_completion.scs`,
`tests/test_agent_link_explanation_to_provenance.cpp` (3 теста),
`agents_vault/AgentLinkExplanationToProvenance.md`.

### 5.20. `AgentRecordStateTransition` — механический учёт перехода состояния запроса (было: §7.1, найдено 03.09.2026 после итогового обсуждения дальнейшего плана работы)

**Проблема была:** §7.1 описывает полное отсутствие агента, который
хранит и изменяет состояние обработки запроса — вся онтологическая
база (`concepts/mediation_lifecycle.scs`: состояния, граф допустимости
`nrel_transitions_to`, реифицированный `concept_state_transition`;
`concepts/processing_plan.scs`: `b_q`/`d_max`) существовала, но ни один
из 26 агентов её не читал и не писал.

**Сделано:** добавлен `AgentRecordStateTransition` — механический
«исполнитель перехода», не решающий оркестратор: вызывающая сторона
передаёт целевой класс состояния (`to_state_class`), агент проверяет
допустимость ребра (`nrel_transitions_to` между классами), ресурсное
ограничение (`b_rem` уменьшается на 1 только на ребре
`x_exec → x_assign`, дословно по `constraint_separate_resource_decrease`,
`axioms/axioms_constraints.scs`) и строит полный пакет перехода —
новый неизменяемый экземпляр состояния, узел
`concept_state_transition` с ролями `rrel_from_state`/`rrel_to_state`/
`rrel_transition_event` и отношениями `nrel_caused_by_event`/
`nrel_uses_plan_version`/`nrel_transition_recorded_in`
(`constraint_transition_provenance`), запись аудита
(`concept_audit_record_transition`) с провенансной записью (по
прецеденту §5.1 `AgentRegisterPartialResult`). Покрывает только
уже покрытый агентами участок конвейера — `x_exec`/`x_aggregate`/
`x_verify` плюс ребро переназначения `x_exec → x_assign`; выбор ЧЕГО и
КОГДА переходить по-прежнему остаётся за вызывающей стороной, тот же
принцип «нет агента, вызывающего другого агента», что и во всём
остальном модуле (подтверждено новым `TEST_F` в
`test_integration_request_pipeline.cpp`, где переход `x_exec →
x_aggregate → x_verify` окружает вызов `AgentValidateIntegration` без
единого оркестратора). Впервые формализовано в KB не как `rule_*`-текст,
а напрямую как `action_record_state_transition` в новом
`rules/rules_mediation_orchestration.scs`, операционализирующем
ограничения `axioms_constraints.scs` без промежуточного `rule_*`.

**Осталось:** §7.1 остаётся открытым в основном — этот агент не решает,
КОГДА и НА КАКОЕ состояние переходить (нет содержательной проверки,
что T(q) действительно завершено для `x_exec → x_aggregate`, например),
не различает переназначение от перепланирования дальше уже покрытого
ребра, не охватывает 9 состояний вне `x_exec`/`x_assign`/`x_aggregate`/
`x_verify` и не запускает цикл перепланирования (`d_max`) — это Фаза 2,
требующая агентов §7.5, которых пока нет. `rrel_transition_actor`/
`rrel_context` на узле перехода сознательно не заполняются (не входят
в минимальный обязательный пакет `constraint_transition_provenance`).

Файлы: `src/agent/agent_record_state_transition.{hpp,cpp}`,
`src/keynodes/lifecycle_keynodes.hpp` (новый),
`rules/rules_mediation_orchestration.scs` (новый),
`tests/test_agent_record_state_transition.cpp` (6 тестов),
`tests/test_integration_request_pipeline.cpp` (расширен — новый
`TEST_F`), `agents_vault/AgentRecordStateTransition.md`.

## 6. Сильные стороны реализации

Раздел не менялся — актуален для всех агентов независимо от статуса выше.

### 6.1. Проверяемые атомарные действия

Каждый агент является `ScActionInitiatedAgent`, проверяет наличие
аргументов и завершает действие с ошибкой при некорректном вызове. Это
делает контракты агентов явными и упрощает тестирование.

### 6.2. Актуальный вектор доверия

Четыре агента обновляют компоненты `T(s,τ)=⟨t_rel,t_acc,t_sec,t_tr⟩`, а
`AgentAggregateTrust` строит агрегированную оценку. Состав компонент
соответствует актуальной модели доверия в диссертации.

### 6.3. Базовый контур кооперативного исполнения

Связка из трёх агентов демонстрирует полезную цепочку: готовая подзадача →
диспетчеризация → получение частичного результата → завершение подзадачи →
передача результата зависимой подзадаче. Эту связку следует сохранить как
часть будущего состояния `x_exec`.

### 6.4. Наличие интеграционных тестов

`tests/test_integration_request_pipeline.cpp` проверяет последовательную
работу нескольких агентов (2 сценария). Хорошая основа, которую нужно
расширить до сценариев актуальной модели с загрузкой реальной KB (§7.7).

## 7. Осталось сделать

Пункты, где сам агент (или отсутствующая подсистема) не менялся вовсе, или
менялась только смежная онтология без изменения агента.

### 7.1. Нет агента-оркестратора жизненного цикла (было: п.6.1)

В актуальной модели запрос проходит состояния:

```text
x_int → x_plan → x_assign → x_exec → x_aggregate → x_verify → x_present
  │                                      │             ├→ x_partial
  └──────────────────────────────→ x_clar └─────────────└→ x_refuse
```

и имеет два различных восстановительных цикла:

```text
x_exec   ──переназначение, b_q──> x_assign
x_assign ──перепланирование, d_max──> x_plan
```

Нет агента, который хранит и изменяет состояние обработки, проверяет
допустимость перехода, выбирает следующий агентный шаг, отличает
переназначение от перепланирования и обеспечивает достижение терминального
исхода.

**Требуется:** `AgentMediationLifecycleCoordinator` или совокупность строго
согласованных агентов переходов состояний. Крупнейший оставшийся разрыв —
от него зависят и §7.3, и §7.5 ниже.

**Онтологическая база уже готова:** `R_ctrl`
(`relations/relations_control.scs`), `ExecutionAct`/`AuditRecord`
(`concepts/execution_acts.scs`), `ProcessingPlan`/`PlanVersion`/`b_q`/
`d_max` (`concepts/processing_plan.scs`), состояния жизненного цикла
(`concepts/mediation_lifecycle.scs`) — всё это существует в KB, но ни один
агент это не читает и не проверяет.

**Частично закрыто §5.20** (03.09.2026): `AgentRecordStateTransition`
механически фиксирует уже принятое решение о переходе (проверяет
`nrel_transitions_to` и `b_rem`, строит пакет провенанса перехода) для
участка `x_exec`/`x_assign`/`x_aggregate`/`x_verify` — но не решает,
какой переход выбрать и когда, и не покрывает остальные 9 состояний
и цикл перепланирования. Полный `AgentMediationLifecycleCoordinator`
(или эквивалент) остаётся крупнейшим оставшимся разрывом.

### 7.2. Провенансный агент не разделён на подсистему (было: часть п.6.7)

Правило `rule_compute_pi_metrics` требует не только создания нарушения при
`π<1` (сделано, §5.3), но и разделения ответственности:

```text
AgentComputePiMetric              → вычисляет метрику (есть);
AgentProvenanceCompletenessValidator → создаёт нарушение полноты (есть, слито с вычислением);
AgentProvenanceRegistrar          → создаёт стандартные записи происхождения (нет);
AgentProvenanceRecovery           → восстанавливает пропущенный след либо маркирует unverifiable (нет).
```

Третья строка теперь частично закрыта: `AgentRecordProvEntity`/
`AgentRecordProvActivity` (§5.15, §5.16) создают именно «стандартные
записи происхождения» — PROV-O триаду `concept_prov_entity`/
`_activity`/`_agent` для результата и производящей его деятельности.
Не хватает по-прежнему автоматического триггера (оба агента вызываются
явно, а не по событию появления результата/исполнения процесса) и
самой `AgentProvenanceRecovery` (`rule_fill_missing_provenance` —
эвристика без алгоритма, остаётся PARTIAL). Единого агента с именем
`AgentProvenanceRegistrar` в проекте нет — есть отдельные `AgentRecordProvEntity`/
`AgentRecordProvActivity`, тот же паттерн, что и у `AgentVerifyResult`
(§7.5): заявленная область разделена между несколькими существующими
агентами.

### 7.3. `AgentDispatchReadySubtask` не подтверждает допустимость исполнения (было: п.6.3)

Агент считает подзадачу готовой, если назначен исполнитель, статус равен
`proposed`, а все зависимости завершены — и корректно реализует именно это:
у `rule_dispatch_ready_subtask` в KB нет условия допустимости вообще, кроме
этих трёх. Он не проверяет: контекстную совместимость; профиль
возможностей; доступность исполнителя; разрешение передачи данных;
действительность версии плана; наличие бюджета и политики исполнения.
Также генерируемый `KnowledgeExchangeAct` не содержит `rrel_context` и
`rrel_timestamp` — источник для них не определён.

**Проверено по гл. 2 диссертации (part2.tex, формула допустимости
назначения):**
`assign(σ,s)=1 ⟺ match(σ,s|C)≥θ ∧ valid(C,L_s)=1 ∧ permit(view(C,s),s,C)=1
∧ available(s,C)=1`. Полная проверка допустимости — работа
`agent_task_allocator`/`AgentAllocateSubtasks` (§7.5, не реализован):
пересчитывать `assign(σ,s)` заново при выдаче означало бы дублировать
распределителя, которого в проекте нет — у агента нет ни контекста `C`, ни
сигнала `available(s,C)`.

**Сделана только смежная онтологическая правка:** сама формула
`assign(σ,s)` в KB (`concepts/capabilities.scs`,
`affirmation_assign_admissibility`) была устаревшей (содержала только
`match ≥ θ ∧ valid(C,L_s)`, без `permit`/`available`) — актуализирована
под текущую редакцию гл. 2. Агент при этом не менялся.

### 7.4. Агрегация доверия не имеет контекста, версии и провенанса (было: п.6.6)

`AgentAggregateTrust` получает пять узлов и записывает число в первый
найденный или новый link. Значение не связано явно с участником сообщества,
типом задачи, версией контекста, временной меткой, основанием вычисления,
версией весовой политики.

**Исследовано, архитектурно заблокировано.** По гл. 2 `T(s,τ)` отнесён к
управляющим отношениям `R_ctrl`, а не к провенансным `R_prov` — навешивать
сюда `ProvenanceRecord` значило бы нарушать уже зафиксированный домен
`nrel_has_provenance` (`concept_processing_result ∪ concept_audit_record`,
`relations/relations_provenance.scs`). "Версия весовой политики" по гл. 2 —
фиксированный экспериментальный параметр раздела верификации диссертации,
а не факт, записываемый в KB при каждом вызове. Связь с участником/типом
задачи технически уже доступна через существующие `nrel_has_trust_level` и
`nrel_trust_for_task` (`relations/relations_trust.scs`), но делать их
обязательными в этом агенте сломало бы текущее использование `trust_level`
в тестах и сценариях (создаётся заранее непривязанным), а сама формула
`rule_aggregate_trust` такого условия не ставит. Требует архитектурного
решения (например, версионирование `trust_level` по `τ`), а не точечной
правки агента.

### 7.5. Отсутствующие агенты (было: §7)

| Агент / действие | Назначение |
|---|---|
| `AgentInterpretRequest` | Формирует намерение, выявляет неоднозначность и инициирует уточнение. |
| `AgentBuildPlan` | Строит DAG подзадач и создаёт исходную версию плана. |
| `AgentValidatePlan` | Проверяет ацикличность, полноту и допустимость плана. |
| `AgentAllocateSubtasks` | Выбирает исполнителей по `match`, политикам доступа и доступности — блокирует §7.3, §7.4. |
| `AgentExecutionMonitor` | Отслеживает завершение, тайм-аут, недоступность и нарушения. |
| `AgentReassignSubtask` | Использует бюджет `b_q`, меняет исполнителя, не меняя структуру плана. |
| `AgentReplanRequest` | Использует `d_max`, создаёт новую версию плана. |
| `AgentRegisterExecutionAudit` | Создаёт `ExecutionAct`, `AuditRecord`, `ProvenanceRecord` — частично закрыто `AgentRegisterPartialResult` (§5.1). |
| `AgentAggregateResults` | Создаёт кандидат итогового результата из частичных результатов. |
| `AgentVerifyResult` | Проверяет ограничения, объяснение, провенанс и конфликты. |
| `AgentResolveConflict` | Создаёт `ConflictResolutionAct`, выбирает стратегию и фиксирует основания — частично закрыто `AgentResolveByTrustWeighted` (§5.2, только стратегия доверия). |
| `AgentPublishResult` | Допускает представление результата пользователю только из допустимого состояния. |
| `AgentEscalateToHuman` | Передаёт спорный либо критичный случай человеку или эксперту. |

`AgentBuildExplanation` из этого списка перенесён в §5.6 — реализована
структурная реификация, семантическое наполнение (агрегация/различия/
минимальное изменение) осталось открытым. Часть заявленной области
`AgentVerifyResult` — explanation + conflicts (`rule_validate_integration`) —
закрыта отдельным агентом `AgentValidateIntegration` (§5.7); ограничения
и провенанс из той же строки покрываются независимо
`AgentValidateCardinalityConstraint` (§5.4) и `AgentComputePiMetric`
(§5.3). Единого агента с именем `AgentVerifyResult` в проекте нет —
его заявленная область разделена между тремя существующими агентами,
каждый из которых закрывает её лишь частично. Аналогично, часть заявленной
области `AgentValidatePlan` — «проверяет ацикличность» — закрыта
`AgentCheckAcyclicity` (§5.11, `rule_check_acyclicity`); «полнота» и
«допустимость плана» (`rule_assignment_admissibility`,
`rules_allocation.scs`) остаются нереализованными — тот же класс
блокировки, что у §7.3/§7.4.

### 7.6. Архитектурная реорганизация модуля (было: §8)

Текущее имя `TrustModule` и библиотека `ostis-ps-trust` не отражают
фактический состав агентов: в модуле уже находятся доверие, безопасность,
целостность, провенанс, интеграция и кооперативное исполнение.

**Рекомендация аудита:** после появления жизненного цикла перейти к одному
модулю `MediatorModule` (trust/lifecycle/execution/provenance/integrity/
integration/security) — не меняет архитектурный замысел, но устраняет
ложное впечатление, что реализован только доверительный модуль. Не
реализовано.

### 7.7. Сквозные тесты на реальной KB (было: п.9.2)

Текущие тесты проверяют корректность C++-алгоритмов на программно
сгенерированных sc-элементах — это не доказывает согласованность с
реальной KB `ostis_kb_user_ecosystem` (§6.4 — интеграционные тесты есть, но
без загрузки KB). Не хватало:

1. ~~Теста загрузки реальной KB и поиска всех keynodes.~~ — ✅ **сделано**
   2026-09-03, `tests/test_load_real_kb.cpp`. Единственный из десяти пунктов,
   не зависящий от отсутствующего оркестратора жизненного цикла (§7.1) —
   остальные девять требуют сквозного маршрута запроса, которого без него
   нет. Технически нетривиален: `ScKeynode::Initialize`
   (sc-memory/sc_keynodes.cpp) резолвит идентификаторы по схеме
   find-OR-CREATE, а не find-or-fail, и делает это безусловно внутри
   `ScMemory::Initialize()` для вообще всех статически сконструированных
   в процессе `ScKeynode` — значит, проверка `IsElement(...)` после
   инициализации не может отличить «уже существовало» от «только что
   молча создано». Решение — сравнение общего количества элементов KB:
   `tools/kb_baseline_counter.cpp`, отдельный исполняемый файл,
   собираемый ТОЛЬКО с `sc-machine::sc-memory` (ни один из
   `src/keynodes/*.hpp` проекта в него не слинкован), даёт «базовую»
   численность — KB плюс встроенные keynodes sc-machine, но без единого
   keynode этого проекта. Основной тест грузит ту же самую сборку KB, но
   уже со всем `TrustModule` (значит, со всеми keynodes проекта), и
   сравнивает счётчик с базовым — расхождение означало бы, что какой-то
   `*_keynodes.hpp`-идентификатор разошёлся с реальной KB (опечатка,
   переименованная связь) и был молча досоздан вместо резолва.
   Копия KB для теста собирается `sc-builder`ом в
   `${CMAKE_BINARY_DIR}/kb_test_repo` — внутри build-дерева, отдельно от
   продакшен-каталога `ostis_ps_user_ecosystem/repo/`, который реально
   раздаёт sc-server; загружается с `clear=SC_FALSE` и выгружается с
   `saveState=SC_FALSE`, то есть никогда не мутирует и не стирает ничего
   за пределами своей одноразовой копии — проверено явно: контрольная
   сумма `repo/` до и после сборки идентична.
2. Успешного сквозного маршрута от запроса до `x_present`.
3. Неоднозначного запроса и перехода в `x_clar`.
4. Тайм-аута исполнителя и переназначения с уменьшением `b_q`.
5. Исчерпания `b_q` и перехода к перепланированию.
6. Исчерпания `d_max` и структурированного отказа `⟨c,varrho,a⟩`.
7. Неразрешённого конфликта и запрета публикации результата.
8. Разрешённого конфликта, сохранённого в провенансе.
9. Запрета доступа и отсутствия передачи персональных данных.
10. Проверки, что каждый `ExecutionAct` имеет `AuditRecord` и `ProvenanceRecord`.

Пункты 2–10 по-прежнему не добавлены — все требуют сквозного маршрута
запроса через агента-оркестратора жизненного цикла, которого нет (§7.1);
не закрываются точечно, тем же классом блокировки, что и остальные
пункты §7.

## 8. Рекомендуемый порядок доработки

### Этап 1. Онтология — в основном готово

Все шесть исходных пунктов Этапа 1 (`R_ctrl`; разведение `AuditRecord`/
`ProvenanceRecord`; `ExecutionAct`/`ProcessingPlan`/`PlanVersion`;
состояния жизненного цикла; `b_q`/`d_max`; модель конфликта) уже
присутствуют в KB, а формула `assign(σ,s)` актуализирована под текущую
гл. 2 (§7.3). Потребляющих эту онтологию агентов нет — этим объясняется
блокировка §7.1 и §7.3–7.4.

### Этап 2. Актуализация существующих агентов — сделана частично

Из шести исходных пунктов: диапазоны доверия (§4.1), разрешение конфликтов
отдельным актом (§5.2) и регистрация в CTest (§4.2) — сделаны; регистрация
результата под `ExecutionAct` — сделана (§5.1), диспетчеризация — нет
(§7.3); аудит/провенанс встроены в регистрацию результата и проверку
полноты провенанса, но не в диспетчеризацию, агрегацию доверия и
детектирование инцидента; детектирование инцидента получило
классификацию, но не связь с управляющей реакцией (§5.5).

### Этап 3. Контур жизненного цикла — не начат

1. Координатор состояний (§7.1).
2. Построение и валидация плана (§7.5).
3. Назначение, мониторинг и восстановление (§7.5).
4. Верификация, объяснение и публикация (§7.5) — структурная реификация
   объяснения (§5.6) и проверка explanation+conflicts перед публикацией
   (§5.7) уже сделаны; проверка ограничений/провенанса встроена в §5.3/§5.4
   отдельно, а не в единый `AgentVerifyResult`; сама публикация — нет.
5. Интеграционные тесты на реальной KB (§7.7).

## 9. Минимальный критерий готовности агентного слоя

Не менялся — остаётся целевым чек-листом:

- каждый существенный этап обработки связан с состоянием жизненного цикла — механизм фиксации есть (`AgentRecordStateTransition`, §5.20), но только для уже покрытого участка `x_exec`/`x_assign`/`x_aggregate`/`x_verify`, и ничто пока не решает, когда его вызывать;
- назначение проверяет возможности, контекст, доступность и политику доступа;
- каждый факт исполнения фиксируется как `ExecutionAct`, `AuditRecord` и `ProvenanceRecord`;
- переназначение и перепланирование используют разные ресурсы `b_q` и `d_max`;
- конфликт не приводит к автоматической публикации результата;
- публикация возможна только после верификации, объяснения и отсутствия неразрешённого конфликта — предикат вычисляется (`AgentValidateIntegration`, §5.7), но самой публикации (`AgentPublishResult`), которая обязана его проверить перед показом пользователю, ещё нет;
- отказ содержит код, `varrho` и запись аудита;
- значения доверия не выходят за `[0,1]` — **выполнено** (§4.1);
- все тесты запускаются через CTest — **выполнено** (§4.2);
- есть сквозные тесты с фактически загруженной онтологией — не выполнено (§7.7).

## 10. Итоговое решение

Существующие агенты — ценный и рабочий задел, переписывать их не нужно.
Из десяти пунктов раздела о критических разрывах (было §6.1–6.10): три
закрыты полностью (§4), пять — частично, с явно очерченным остатком (§5),
два (диспетчеризация и агрегация доверия) уперлись не в отсутствие
онтологии, а в отсутствие координатора/распределителя — их нельзя закрыть
точечной правкой агента без архитектурного решения по жизненному циклу.

Единственный оставшийся разрыв, который требует именно нового кода (а не
актуализации существующего), — координатор жизненного цикла (§7.1) и
связанные с ним отсутствующие агенты (§7.5), в первую очередь
`agent_task_allocator`, от которого зависят и диспетчеризация (§7.3), и
агрегация доверия (§7.4).

Первый срез этого разрыва закрыт (§5.20, 03.09.2026): механическая
фиксация перехода состояния для уже покрытого участка конвейера. Это
целенаправленно не решающий оркестратор — выбор, КОГДА и НА КАКОЕ
состояние переходить, по-прежнему нигде не реализован, и именно это,
а не механика фиксации, остаётся содержательным ядром §7.1.
